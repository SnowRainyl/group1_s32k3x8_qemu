#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/module.h"
#include "hw/irq.h"
#include "s32k3x8_uart.h"
#include "hw/registerfields.h"
#include "hw/qdev-core.h"
#include "chardev/char.h"
#include "chardev/char-fe.h"
#include "hw/char/ibex_uart.h"
#include "hw/qdev-clock.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"



/* Debug logging */
#define UART_DEBUG 0
#if UART_DEBUG
#define DPRINTF(fmt, ...) \
    fprintf(stderr, "s32k3x8_lpuart: " fmt, ## __VA_ARGS__)
#else
#define DPRINTF(fmt, ...) do {} while (0)
#endif

static void s32k3x8_lpuart_update_irq(S32K3X8LPUARTState *s)
{
    bool irq_state = false;

    if ((s->ctrl & S32K3X8_LPUART_CTRL_RXIE) && 
        (s->stat & S32K3X8_LPUART_STAT_RXRDY)) {
        irq_state = true;
    }

    if ((s->ctrl & S32K3X8_LPUART_CTRL_TXIE) && 
        (s->stat & S32K3X8_LPUART_STAT_TXRDY)) {
        irq_state = true;
    }

    qemu_set_irq(s->irq, irq_state);
    DPRINTF("IRQ state changed to %d\n", irq_state);
}

static const VMStateDescription vmstate_s32k3x8_lpuart = {
    .name = "s32k3x8-lpuart",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT8(instance_id, S32K3X8LPUARTState),
        VMSTATE_UINT32(verid, S32K3X8LPUARTState),
        VMSTATE_UINT32(param, S32K3X8LPUARTState),
        VMSTATE_UINT32(global, S32K3X8LPUARTState),
        VMSTATE_UINT32(pincfg, S32K3X8LPUARTState),
        VMSTATE_UINT32(baud, S32K3X8LPUARTState),
        VMSTATE_UINT32(stat, S32K3X8LPUARTState),
        VMSTATE_UINT32(ctrl, S32K3X8LPUARTState),
        VMSTATE_UINT32(data, S32K3X8LPUARTState),
        VMSTATE_UINT32(match, S32K3X8LPUARTState),
        VMSTATE_UINT32(modir, S32K3X8LPUARTState),
        VMSTATE_UINT32(fifo, S32K3X8LPUARTState),
        VMSTATE_UINT32(water, S32K3X8LPUARTState),
        VMSTATE_UINT32(dataro, S32K3X8LPUARTState),
        VMSTATE_UINT32(mcr, S32K3X8LPUARTState),
        VMSTATE_UINT32(msr, S32K3X8LPUARTState),
        VMSTATE_UINT32(reir, S32K3X8LPUARTState),
        VMSTATE_UINT32(teir, S32K3X8LPUARTState),
        VMSTATE_UINT32(hdcr, S32K3X8LPUARTState),
        VMSTATE_UINT32(tocr, S32K3X8LPUARTState),
        VMSTATE_UINT32(tosr, S32K3X8LPUARTState),
        VMSTATE_UINT32_ARRAY(timeout, S32K3X8LPUARTState, 4),
        VMSTATE_UINT32_ARRAY(tcbr, S32K3X8LPUARTState, 128),
        VMSTATE_UINT32_ARRAY(tdbr, S32K3X8LPUARTState, 256),
        VMSTATE_END_OF_LIST()
    }
};

static uint64_t s32k3x8_lpuart_read(void *opaque, hwaddr offset, unsigned size)
{
    S32K3X8LPUARTState *s = (S32K3X8LPUARTState *)opaque;
    uint32_t value = 0;

    if (!(s->global & S32K3X8_LPUART_GLOBAL_EN)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                     "s32k3x8_lpuart: Read while LPUART disabled, offset %"HWADDR_PRIx"\n",
                     offset);
        return 0;
    }

    switch (offset) {
    case S32K3X8_LPUART_VERID:
        value = s->verid;
        break;
    case S32K3X8_LPUART_PARAM:
        value = s->param;
        break;
    case S32K3X8_LPUART_GLOBAL:
        value = s->global;
        break;
    case S32K3X8_LPUART_PINCFG:
        value = s->pincfg;
        break;
    case S32K3X8_LPUART_BAUD:
        value = s->baud;
        break;
    case S32K3X8_LPUART_STAT:
        value = s->stat;
        break;
    case S32K3X8_LPUART_CTRL:
        value = s->ctrl;
        break;
    case S32K3X8_LPUART_DATA:
        value = s->data;
        if (s->stat & S32K3X8_LPUART_STAT_RXRDY) {
            s->stat &= ~S32K3X8_LPUART_STAT_RXRDY;
            s32k3x8_lpuart_update_irq(s);
        }
        break;
    case S32K3X8_LPUART_MATCH:
        value = s->match;
        break;
    case S32K3X8_LPUART_MODIR:
        value = s->modir;
        break;
    case S32K3X8_LPUART_FIFO:
        value = s->fifo;
        break;
    case S32K3X8_LPUART_WATER:
        value = s->water;
        break;
    case S32K3X8_LPUART_DATARO:
        value = s->dataro;
        break;
    case S32K3X8_LPUART_MCR:
        value = s->mcr;
        break;
    case S32K3X8_LPUART_MSR:
        value = s->msr;
        break;
    case S32K3X8_LPUART_REIR:
        value = s->reir;
        break;
    case S32K3X8_LPUART_TEIR:
        value = s->teir;
        break;
    case S32K3X8_LPUART_HDCR:
        value = s->hdcr;
        break;
    case S32K3X8_LPUART_TOCR:
        value = s->tocr;
        break;
    case S32K3X8_LPUART_TOSR:
        value = s->tosr;
        break;
    default:
        if (offset >= S32K3X8_LPUART_TIMEOUT0 && 
            offset < S32K3X8_LPUART_TIMEOUT0 + 0x10) {
            value = s->timeout[(offset - S32K3X8_LPUART_TIMEOUT0) >> 2];
        } else if (offset >= S32K3X8_LPUART_TCBR0 && 
                   offset < S32K3X8_LPUART_TCBR0 + 0x200) {
            value = s->tcbr[(offset - S32K3X8_LPUART_TCBR0) >> 2];
        } else if (offset >= S32K3X8_LPUART_TDBR0 && 
                   offset < S32K3X8_LPUART_TDBR0 + 0x400) {
            value = s->tdbr[(offset - S32K3X8_LPUART_TDBR0) >> 2];
        } else {
            qemu_log_mask(LOG_GUEST_ERROR,
                         "s32k3x8_lpuart: Bad read offset %"HWADDR_PRIx"\n", 
                         offset);
        }
    }
DPRINTF("Read: offset=%"HWADDR_PRIx" value=0x%x\n", offset, value);
    return value;
}


static void s32k3x8_lpuart_write(void *opaque, hwaddr offset,
                              uint64_t value, unsigned size)
{
    S32K3X8LPUARTState *s = (S32K3X8LPUARTState *)opaque;

    DPRINTF("Write: offset=%"HWADDR_PRIx" value=0x%"PRIx64"\n", offset, value);

    if (!(s->global & S32K3X8_LPUART_GLOBAL_EN) && 
        offset != S32K3X8_LPUART_GLOBAL) {
        qemu_log_mask(LOG_GUEST_ERROR,
                     "s32k3x8_lpuart: Write while LPUART disabled, offset %"HWADDR_PRIx"\n",
                     offset);
        return;
    }

    switch (offset) {
    case S32K3X8_LPUART_GLOBAL:
        s->global = value;
        if (value & S32K3X8_LPUART_GLOBAL_RST) {
            device_cold_reset(DEVICE(s));
        }
        break;
    case S32K3X8_LPUART_PINCFG:
        s->pincfg = value;
        break;
    case S32K3X8_LPUART_BAUD:
        s->baud = value;
        break;
    case S32K3X8_LPUART_STAT:
        /* Write-1-to-clear error flags */
        s->stat &= ~(value & S32K3X8_LPUART_STAT_ERR_MASK);
        s32k3x8_lpuart_update_irq(s);
        break;
    case S32K3X8_LPUART_CTRL:
        s->ctrl = value;
        s32k3x8_lpuart_update_irq(s);
        break;
    case S32K3X8_LPUART_DATA:
        s->data = value;
        if (qemu_chr_fe_backend_connected(&s->chr)) {
            uint8_t ch = value & 0xFF;
            qemu_chr_fe_write(&s->chr, &ch, 1);
        }
        /* Set TXRDY after transmission */
        s->stat |= S32K3X8_LPUART_STAT_TXRDY;
        s32k3x8_lpuart_update_irq(s);
        break;
    case S32K3X8_LPUART_MATCH:
        s->match = value;
        break;
    case S32K3X8_LPUART_MODIR:
        s->modir = value;
        break;
    case S32K3X8_LPUART_FIFO:
        s->fifo = value;
        break;
    case S32K3X8_LPUART_WATER:
        s->water = value;
        break;
    case S32K3X8_LPUART_MCR:
        s->mcr = value;
        break;
    case S32K3X8_LPUART_MSR:
        s->msr = value;
        break;
    case S32K3X8_LPUART_REIR:
        s->reir = value;
        break;
    case S32K3X8_LPUART_TEIR:
        s->teir = value;
        break;
    case S32K3X8_LPUART_HDCR:
        s->hdcr = value;
        break;
    case S32K3X8_LPUART_TOCR:
        s->tocr = value;
        break;
    case S32K3X8_LPUART_TOSR:
        s->tosr = value;
        break;
    default:
        if (offset >= S32K3X8_LPUART_TIMEOUT0 && 
            offset < S32K3X8_LPUART_TIMEOUT0 + 0x10) {
            s->timeout[(offset - S32K3X8_LPUART_TIMEOUT0) >> 2] = value;
        } else if (offset >= S32K3X8_LPUART_TCBR0 && 
                   offset < S32K3X8_LPUART_TCBR0 + 0x200) {
            s->tcbr[(offset - S32K3X8_LPUART_TCBR0) >> 2] = value;
        } else if (offset >= S32K3X8_LPUART_TDBR0 && 
                   offset < S32K3X8_LPUART_TDBR0 + 0x400) {
            s->tdbr[(offset - S32K3X8_LPUART_TDBR0) >> 2] = value;
        } else {
            qemu_log_mask(LOG_GUEST_ERROR,
                         "s32k3x8_lpuart: Bad write offset %"HWADDR_PRIx"\n",
                         offset);
        }
    }
}

static const MemoryRegionOps s32k3x8_lpuart_ops = {
    .read = s32k3x8_lpuart_read,
    .write = s32k3x8_lpuart_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void s32k3x8_lpuart_reset(DeviceState *dev)
{
    S32K3X8LPUARTState *s = S32K3X8_LPUART(dev);
    
    /* Set reset values based on instance ID */
    if (s->instance_id <= 1) {
        s->verid = S32K3X8_LPUART_VERID_RESET01;  /* VERID for LPUART0/1 */
        s->param = S32K3X8_LPUART_PARAM_RESET01;  /* PARAM for LPUART0/1 */
        s->fifo = S32K3X8_LPUART_FIFO_RESET01;   /* FIFO for LPUART0/1 */
    } else {
        s->verid = S32K3X8_LPUART_VERID_RESET;   /* VERID for LPUART2-15 */
        s->param = S32K3X8_LPUART_PARAM_RESET;   /* PARAM for LPUART2-15 */
        s->fifo = S32K3X8_LPUART_FIFO_RESET;     /* FIFO for LPUART2-15 */
    }

    /* Common reset values for all instances */
    s->global = 0x00000000;    /* Global */
    s->pincfg = 0x00000000;    /* Pin Configuration */
    s->baud = 0x0F000004;      /* Baud Rate */
    s->stat = 0x00C00000;      /* Status */
    s->ctrl = 0x00000000;      /* Control */
    s->data = 0x00001000;      /* Data */
    s->match = 0x00000000;     /* Match Address */
    s->modir = 0x00000000;     /* MODEM IrDA */
    s->water = 0x00000000;     /* Watermark */
    s->dataro = 0x00001000;    /* Data Read-Only */
    s->mcr = 0x00000000;       /* MODEM Control */
    s->msr = 0x00000000;       /* MODEM Status */
    s->reir = 0x00000000;      /* Receiver Extended Idle */
    s->teir = 0x00000000;      /* Transmitter Extended Idle */
    s->hdcr = 0x00000000;      /* Half Duplex Control */
    s->tocr = 0x00000000;      /* Timeout Control */
    s->tosr = 0x0000000F;      /* Timeout Status */
    
    memset(s->timeout, 0, sizeof(s->timeout));
    memset(s->tcbr, 0, sizeof(s->tcbr));
    memset(s->tdbr, 0, sizeof(s->tdbr));

    s32k3x8_lpuart_update_irq(s);
}

static int s32k3x8_lpuart_can_receive(void *opaque)
{
    S32K3X8LPUARTState *s = (S32K3X8LPUARTState *)opaque;
    return !(s->stat & S32K3X8_LPUART_STAT_RXRDY) && 
           (s->ctrl & S32K3X8_LPUART_CTRL_RX_EN);
}

static void s32k3x8_lpuart_receive(void *opaque, const uint8_t *buf, int size)
{
    S32K3X8LPUARTState *s = (S32K3X8LPUARTState *)opaque;
    
    if (!(s->ctrl & S32K3X8_LPUART_CTRL_RX_EN)) {
        return;
    }

    s->data = *buf;
    s->stat |= S32K3X8_LPUART_STAT_RXRDY;
    s32k3x8_lpuart_update_irq(s);
}

static void s32k3x8_lpuart_realize(DeviceState *dev, Error **errp)
{
	
    printf("realize!\n");
    S32K3X8LPUARTState *s = S32K3X8_LPUART(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    printf("Device pointer: %p\n", dev);
printf("State pointer: %p\n", s);
printf("SysBus pointer: %p\n", sbd);
    if (!s || !sbd) {
        printf("Failed to initialize LPUART device!\n");
        return;
    }


    memory_region_init_io(&s->iomem, OBJECT(s), &s32k3x8_lpuart_ops, s,
                         TYPE_S32K3X8_LPUART, 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    qemu_chr_fe_set_handlers(&s->chr, s32k3x8_lpuart_can_receive,
                            s32k3x8_lpuart_receive, NULL, NULL,
                            s, NULL, true);
}

static Property s32k3x8_lpuart_properties[] = {
    DEFINE_PROP_UINT8("instance", S32K3X8LPUARTState, instance_id, 0),
    DEFINE_PROP_CHR("chardev", S32K3X8LPUARTState, chr),
    DEFINE_PROP_END_OF_LIST(),
};

static void s32k3x8_lpuart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
     if (!dc) {
        printf("Failed to initialize device class");
        return;
    }
     printf("class_init!\n");
    dc->realize = s32k3x8_lpuart_realize;
    printf("Setting realize function: %p\n", s32k3x8_lpuart_realize);
    printf("finish_realize!\n");
    device_class_set_legacy_reset(dc, s32k3x8_lpuart_reset);
    printf("finish reset!\n");
    device_class_set_props(dc, s32k3x8_lpuart_properties);
     printf("finish properties!\n");
    dc->vmsd = &vmstate_s32k3x8_lpuart;
     printf("finish class_init!\n");


}
static const TypeInfo s32k3x8_lpuart_info = {
    .name          = TYPE_S32K3X8_LPUART,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S32K3X8LPUARTState),
    .class_size    = sizeof(S32K3X8LPUARTClass),
    .class_init    = s32k3x8_lpuart_class_init,
};


void s32k3x8_lpuart_register_types(void)
{
	printf("Registering S32K3X8EVB uart type!\n");

    type_register_static(&s32k3x8_lpuart_info); 
    printf("Registeration complete S32K3X8EVB uart type!\n");

}

type_init(s32k3x8_lpuart_register_types)
