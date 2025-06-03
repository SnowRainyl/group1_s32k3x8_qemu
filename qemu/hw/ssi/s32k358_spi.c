/*
 * S32K3X8 LPSPI (Low Power Serial Peripheral Interface) Emulation
 * Based on S32K3xx Reference Manual, Rev. 9, 07/2024
 * 
 */

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/qdev-properties.h"
#include "hw/irq.h"
#include "s32k358_spi.h"
#include "trace.h"

/* FIFO operation functions */
static bool fifo_is_empty(S32K3X8LPSPIState *s, bool tx)
{
    if (tx) {
        return s->tx_fifo.level == 0;
    } else {
        return s->rx_fifo.level == 0;
    }
}

static bool fifo_is_full(S32K3X8LPSPIState *s, bool tx)
{
    if (tx) {
        return s->tx_fifo.level >= LPSPI_FIFO_SIZE;
    } else {
        return s->rx_fifo.level >= LPSPI_FIFO_SIZE;
    }
}

static void fifo_push(S32K3X8LPSPIState *s, uint32_t data, bool tx)
{
    if (tx) {
        if (!fifo_is_full(s, true)) {
            s->tx_fifo.data[s->tx_fifo.tail] = data;
            s->tx_fifo.tail = (s->tx_fifo.tail + 1) % LPSPI_FIFO_SIZE;
            s->tx_fifo.level++;
        }
    } else {
        if (!fifo_is_full(s, false)) {
            s->rx_fifo.data[s->rx_fifo.tail] = data;
            s->rx_fifo.tail = (s->rx_fifo.tail + 1) % LPSPI_FIFO_SIZE;
            s->rx_fifo.level++;
        }
    }
}

static uint32_t fifo_pop(S32K3X8LPSPIState *s, bool tx)
{
    uint32_t data = 0;
    
    if (tx) {
        if (!fifo_is_empty(s, true)) {
            data = s->tx_fifo.data[s->tx_fifo.head];
            s->tx_fifo.head = (s->tx_fifo.head + 1) % LPSPI_FIFO_SIZE;
            s->tx_fifo.level--;
        }
    } else {
        if (!fifo_is_empty(s, false)) {
            data = s->rx_fifo.data[s->rx_fifo.head];
            s->rx_fifo.head = (s->rx_fifo.head + 1) % LPSPI_FIFO_SIZE;
            s->rx_fifo.level--;
        }
    }
    
    return data;
}

static void fifo_reset(S32K3X8LPSPIState *s, bool tx)
{
    if (tx) {
        s->tx_fifo.level = 0;
        s->tx_fifo.head = 0;
        s->tx_fifo.tail = 0;
        memset(s->tx_fifo.data, 0, sizeof(s->tx_fifo.data));
    } else {
        s->rx_fifo.level = 0;
        s->rx_fifo.head = 0;
        s->rx_fifo.tail = 0;
        memset(s->rx_fifo.data, 0, sizeof(s->rx_fifo.data));
    }
}

/* Update status register */
static void s32k3x8_lpspi_update_status(S32K3X8LPSPIState *s)
{
    /* Update FIFO status */
    uint32_t txwater = s->fcr & 0x3;  /* TX watermark */
    uint32_t rxwater = (s->fcr >> 16) & 0x3;  /* RX watermark */
    
    /* TDF: Transmit Data Flag */
    if (s->tx_fifo.level <= txwater) {
        s->sr |= LPSPI_SR_TDF;
    } else {
        s->sr &= ~LPSPI_SR_TDF;
    }
    
    /* RDF: Receive Data Flag */
    if (s->rx_fifo.level > rxwater) {
        s->sr |= LPSPI_SR_RDF;
    } else {
        s->sr &= ~LPSPI_SR_RDF;
    }
    
    /* Update FIFO Status Register */
    s->fsr = (s->rx_fifo.level << 16) | s->tx_fifo.level;
}

/* Update interrupt status */
static void s32k3x8_lpspi_update_irq(S32K3X8LPSPIState *s)
{
    bool irq_state = false;
    
    /* Check various interrupt conditions */
    if ((s->ier & 0x1) && (s->sr & LPSPI_SR_TDF)) {        /* TDIE & TDF */
        irq_state = true;
    }
    if ((s->ier & 0x2) && (s->sr & LPSPI_SR_RDF)) {        /* RDIE & RDF */
        irq_state = true;
    }
    if ((s->ier & 0x100) && (s->sr & LPSPI_SR_WCF)) {      /* WCIE & WCF */
        irq_state = true;
    }
    if ((s->ier & 0x200) && (s->sr & LPSPI_SR_FCF)) {      /* FCIE & FCF */
        irq_state = true;
    }
    if ((s->ier & 0x400) && (s->sr & LPSPI_SR_TCF)) {      /* TCIE & TCF */
        irq_state = true;
    }
    
    qemu_set_irq(s->irq, irq_state);
}

/* SPI transfer processing */
static void s32k3x8_lpspi_do_transfer(S32K3X8LPSPIState *s)
{
    if (!s->enabled || !(s->cr & LPSPI_CR_MEN)) {
        return;
    }
    
    /* Simple loopback test implementation */
    if (!fifo_is_empty(s, true) && !fifo_is_full(s, false)) {
        uint32_t tx_data = fifo_pop(s, true);
        
        /* In loopback mode, transmitted data is directly used as received data */
        fifo_push(s, tx_data, false);
        
        /* Set transfer complete flags */
        s->sr |= LPSPI_SR_WCF;  /* Word Complete */
        s->sr |= LPSPI_SR_FCF;  /* Frame Complete */
        
        if (fifo_is_empty(s, true)) {
            s->sr |= LPSPI_SR_TCF;  /* Transfer Complete */
        }
    }
    
    s32k3x8_lpspi_update_status(s);
    s32k3x8_lpspi_update_irq(s);
}

/* Register read operation */
static uint64_t s32k3x8_lpspi_read(void *opaque, hwaddr addr, unsigned size)
{
    S32K3X8LPSPIState *s = S32K3X8_LPSPI(opaque);
    uint64_t ret = 0;
    
    switch (addr) {
    case LPSPI_VERID_OFFSET:
        ret = s->verid;
        break;
    case LPSPI_PARAM_OFFSET:
        ret = s->param;
        break;
    case LPSPI_CR_OFFSET:
        ret = s->cr;
        break;
    case LPSPI_SR_OFFSET:
        s32k3x8_lpspi_update_status(s);
        ret = s->sr;
        break;
    case LPSPI_IER_OFFSET:
        ret = s->ier;
        break;
    case LPSPI_DER_OFFSET:
        ret = s->der;
        break;
    case LPSPI_CFGR0_OFFSET:
        ret = s->cfgr0;
        break;
    case LPSPI_CFGR1_OFFSET:
        ret = s->cfgr1;
        break;
    case LPSPI_DMR0_OFFSET:
        ret = s->dmr0;
        break;
    case LPSPI_DMR1_OFFSET:
        ret = s->dmr1;
        break;
    case LPSPI_CCR_OFFSET:
        ret = s->ccr;
        break;
    case LPSPI_CCR1_OFFSET:
        ret = s->ccr1;
        break;
    case LPSPI_FCR_OFFSET:
        ret = s->fcr;
        break;
    case LPSPI_FSR_OFFSET:
        s32k3x8_lpspi_update_status(s);
        ret = s->fsr;
        break;
    case LPSPI_TCR_OFFSET:
        ret = s->tcr;
        break;
    case LPSPI_TDR_OFFSET:
        /* Write-only register, read returns 0 */
        ret = 0;
        break;
    case LPSPI_RSR_OFFSET:
        /* Receive status register */
        ret = s->rsr;
        if (fifo_is_empty(s, false)) {
            ret |= 0x2;  /* RXEMPTY */
        }
        break;
    case LPSPI_RDR_OFFSET:
        if (!fifo_is_empty(s, false)) {
            ret = fifo_pop(s, false);
            s32k3x8_lpspi_update_status(s);
            s32k3x8_lpspi_update_irq(s);
        }
        break;
    case LPSPI_RDROR_OFFSET:
        /* Read-only, does not remove data from FIFO */
        if (!fifo_is_empty(s, false)) {
            ret = s->rx_fifo.data[s->rx_fifo.head];
        }
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "s32k3x8_lpspi: Read from invalid offset 0x%"HWADDR_PRIx"\n",
                      addr);
        break;
    }
    
    return ret;
}

/* Register write operation */
static void s32k3x8_lpspi_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    S32K3X8LPSPIState *s = S32K3X8_LPSPI(opaque);
    
    switch (addr) {
    case LPSPI_CR_OFFSET:
        s->cr = val;
        
        /* Handle module enable */
        if (val & LPSPI_CR_MEN) {
            s->enabled = true;
        } else {
            s->enabled = false;
        }
        
        /* Handle software reset */
        if (val & LPSPI_CR_RST) {
            /* Reset all registers except control register */
            s->sr = 0x1;  /* Default TDF=1 */
            fifo_reset(s, true);
            fifo_reset(s, false);
        }
        
        /* Handle FIFO reset */
        if (val & LPSPI_CR_RTF) {
            fifo_reset(s, true);
        }
        if (val & LPSPI_CR_RRF) {
            fifo_reset(s, false);
        }
        
        s32k3x8_lpspi_update_status(s);
        s32k3x8_lpspi_update_irq(s);
        break;
        
    case LPSPI_SR_OFFSET:
        /* Write 1 to clear status bits */
        s->sr &= ~(val & 0x3F00);  /* Clear clearable status bits */
        s32k3x8_lpspi_update_irq(s);
        break;
        
    case LPSPI_IER_OFFSET:
        s->ier = val;
        s32k3x8_lpspi_update_irq(s);
        break;
        
    case LPSPI_DER_OFFSET:
        s->der = val;
        break;
        
    case LPSPI_CFGR0_OFFSET:
        s->cfgr0 = val;
        break;
        
    case LPSPI_CFGR1_OFFSET:
        s->cfgr1 = val;
        s->master_mode = !!(val & LPSPI_CFGR1_MASTER);
        break;
        
    case LPSPI_DMR0_OFFSET:
        s->dmr0 = val;
        break;
        
    case LPSPI_DMR1_OFFSET:
        s->dmr1 = val;
        break;
        
    case LPSPI_CCR_OFFSET:
        s->ccr = val;
        break;
        
    case LPSPI_CCR1_OFFSET:
        s->ccr1 = val;
        break;
        
    case LPSPI_FCR_OFFSET:
        s->fcr = val;
        s32k3x8_lpspi_update_status(s);
        s32k3x8_lpspi_update_irq(s);
        break;
        
    case LPSPI_TCR_OFFSET:
        s->tcr = val;
        /* Extract frame size */
        s->transfer_size = (val & LPSPI_TCR_FRAMESZ_MASK) + 1;
        break;
        
    case LPSPI_TDR_OFFSET:
        if (!fifo_is_full(s, true)) {
            fifo_push(s, val, true);
            s32k3x8_lpspi_do_transfer(s);
        }
        break;
        
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "s32k3x8_lpspi: Write to invalid offset 0x%"HWADDR_PRIx"\n",
                      addr);
        break;
    }
}

static const MemoryRegionOps s32k3x8_lpspi_ops = {
    .read = s32k3x8_lpspi_read,
    .write = s32k3x8_lpspi_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4,
    }
};

/* Device reset */
static void s32k3x8_lpspi_reset(DeviceState *dev)
{
    S32K3X8LPSPIState *s = S32K3X8_LPSPI(dev);
    
    /* Set default values according to reference manual */
    s->verid = 0x02000004;      /* Version ID */
    s->param = 0x00040202;      /* Parameters: 4 PCS, 4-word FIFO */
    s->cr = 0x00000000;
    s->sr = 0x00000001;         /* TDF=1 */
    s->ier = 0x00000000;
    s->der = 0x00000000;
    s->cfgr0 = 0x00000000;
    s->cfgr1 = 0x00000000;
    s->dmr0 = 0x00000000;
    s->dmr1 = 0x00000000;
    s->ccr = 0x00000000;
    s->ccr1 = 0x00000000;
    s->fcr = 0x00000000;
    s->fsr = 0x00000000;
    s->tcr = 0x0000001F;        /* Default 32-bit frame */
    s->tdr = 0x00000000;
    s->rsr = 0x00000002;        /* RXEMPTY=1 */
    s->rdr = 0x00000000;
    s->rdror = 0x00000000;
    
    /* Reset internal state */
    s->enabled = false;
    s->master_mode = false;
    s->transfer_size = 32;
    s->current_cs = 0;
    
    /* Reset FIFO */
    fifo_reset(s, true);
    fifo_reset(s, false);
}

/* Device instantiation */
static void s32k3x8_lpspi_instance_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    S32K3X8LPSPIState *s = S32K3X8_LPSPI(obj);
    
    memory_region_init_io(&s->iomem, obj, &s32k3x8_lpspi_ops, s,
                          TYPE_S32K3X8_LPSPI, 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    
    sysbus_init_irq(sbd, &s->irq);
    
    /* Initialize chip select signals */
    for (int i = 0; i < 8; i++) {
        sysbus_init_irq(sbd, &s->cs_lines[i]);
    }
}
static void s32k3x8_lpspi_realize(DeviceState *dev, Error **errp)
{
	s32k3x8_lpspi_reset(dev);

}
/* Device class initialization */
static void s32k3x8_lpspi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    
    dc->realize = s32k3x8_lpspi_realize;
    dc->desc = "S32K3X8 LPSPI Controller";
}

static const TypeInfo s32k3x8_lpspi_info = {
    .name = TYPE_S32K3X8_LPSPI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S32K3X8LPSPIState),
    .instance_init = s32k3x8_lpspi_instance_init,
    .class_init = s32k3x8_lpspi_class_init,
};

void s32k3x8_lpspi_register_types(void)
{
	 printf("Registering S32K3X8 LPSPI type: %s\n", TYPE_S32K3X8_LPSPI);
    type_register_static(&s32k3x8_lpspi_info);
}

type_init(s32k3x8_lpspi_register_types)
