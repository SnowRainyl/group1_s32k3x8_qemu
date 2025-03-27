#include "qemu/osdep.h"
#include "hw/irq.h"
#include "hw/ssi/s32k3x8_lpspi.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "trace.h"

// registers offset
#define LPSPI_VERID   0x00  // Version ID (Read-only)
#define LPSPI_PARAM   0x04  // Parameter Register (Read-only)
#define LPSPI_CR      0x10  // Control Register
#define LPSPI_SR      0x14  // Status Register
#define LPSPI_IER     0x18  // Interrupt Enable Register
#define LPSPI_DER     0x1C  // DMA Enable Register
#define LPSPI_CFGR0   0x20  // Configuration Register 0
#define LPSPI_CFGR1   0x24  // Configuration Register 1
#define LPSPI_DMR0    0x30  // Data Match Register 0
#define LPSPI_DMR1    0x34  // Data Match Register 1
#define LPSPI_CCR     0x40  // Clock Configuration Register
#define LPSPI_CCR1    0x44  // Clock Configuration Register 1
#define LPSPI_FCR     0x58  // FIFO Control Register
#define LPSPI_FSR     0x5C  // FIFO Status Register (Read-only)
#define LPSPI_TCR     0x60  // Transmit Command Register
#define LPSPI_TDR     0x64  // Transmit Data Register (Write-only)
#define LPSPI_RSR     0x70  // Receive Status Register (Read-only)
#define LPSPI_RDR     0x74  // Receive Data Register (Read-only)
#define LPSPI_TCBR    0x3FC // Transmit Command Burst Register (Write-only)


static void s32k3x8_lpspi_reset(DeviceState* dev)
{
    S32K3X8LPSPIState* s = S32K3X8_LPSPI(dev);

    // Explicitly initialize registers to default values from manual
    s->regs[REG_INDEX(LPSPI_VERID)] = 0x02000004;  // Version ID
    s->regs[REG_INDEX(LPSPI_PARAM)] = 0x00080202;  // PARAM (PCSNUM=8, RXFIFO=4, TXFIFO=4 for LPSPI_0)
    s->regs[REG_INDEX(LPSPI_CR)] = 0x00000000;  // Control Register
    s->regs[REG_INDEX(LPSPI_SR)] = 0x00000001;  // Status Register (SR)
    s->regs[REG_INDEX(LPSPI_IER)] = 0x00000000;  // Interrupt Enable Register
    s->regs[REG_INDEX(LPSPI_DER)] = 0x00000000;  // DMA Enable Register
    s->regs[REG_INDEX(LPSPI_CFGR0)] = 0x00000000;  // Configuration Register 0
    s->regs[REG_INDEX(LPSPI_CFGR1)] = 0x00000000;  // Configuration Register 1
    s->regs[REG_INDEX(LPSPI_DMR0)] = 0x00000000;  // Data Match Register 0
    s->regs[REG_INDEX(LPSPI_DMR1)] = 0x00000000;  // Data Match Register 1
    s->regs[REG_INDEX(LPSPI_CCR)] = 0x00000000;  // Clock Configuration Register
    s->regs[REG_INDEX(LPSPI_CCR1)] = 0x00000000;  // Clock Configuration Register 1
    s->regs[REG_INDEX(LPSPI_FCR)] = 0x00000000;  // FIFO Control Register
    s->regs[REG_INDEX(LPSPI_FSR)] = 0x00000000;  // FIFO Status Register
    s->regs[REG_INDEX(LPSPI_TCR)] = 0x0000001F;  // Transmit Command Register
    s->regs[REG_INDEX(LPSPI_RSR)] = 0x00000002;  // Receive Status Register
    s->regs[REG_INDEX(LPSPI_TDR)] = 0x00000000;  // Transmit Data Register (Write-only, reset not critical)
    s->regs[REG_INDEX(LPSPI_RDR)] = 0x00000000;  // Receive Data Register (Read-only, reset to 0)
    s->regs[REG_INDEX(LPSPI_TCBR)] = 0x00000000;  // Transmit Command Burst Register (Write-only, reset not critical)

    // Reset FIFOs explicitly
    fifo32_reset(&s->tx_fifo);
    fifo32_reset(&s->rx_fifo);

    // Update IRQ status after reset explicitly
    s32k3x8_lpspi_update_irq_


static void s32k3x8_lpspi_update_cs(S32K3X8LPSPIState* s, int cs_line, bool level)
{
    qemu_set_irq(s->cs_lines[cs_line], level);
}


static void s32k3x8_lpspi_update_irq(S32K3X8LPSPIState* s)
{
    uint32_t irq_pending;

    /* Check Interrupt Enable Register (IER) and Status Register (SR) */
    irq_pending = s->regs[REG_INDEX(LPSPI_SR)] & s->regs[REG_INDEX(LPSPI_IER)];

    /* Set or clear the IRQ line */
    if (irq_pending) {
        qemu_irq_raise(s->irq);
    }
    else {
        qemu_irq_lower(s->irq);
    }
}


static void s32k3x8_lpspi_handle_transfer(S32K3X8LPSPIState* s)
{
    uint32_t tx_data;
    uint32_t rx_data;

    /* Check if the peripheral is enabled */
    if (!(s->regs[REG_INDEX(LPSPI_CR)] & 0x01)) { // MEN bit
        return;
    }

    /* Process all available data in the TX FIFO */
    while (!fifo32_is_empty(&s->tx_fifo)) {

        /* Pop data from TX FIFO */
        tx_data = fifo32_pop(&s->tx_fifo);

        /* Perform SPI transfer */
        rx_data = ssi_transfer(s->bus, tx_data);

        /* Check if RX FIFO has space */
        if (!fifo32_is_full(&s->rx_fifo)) {
            fifo32_push(&s->rx_fifo, rx_data);
        }
        else {
            /* Set RX FIFO overflow flag in Status Register (SR) */
            s->regs[REG_INDEX(LPSPI_SR)] |= (1 << 12);
        }

        /* Set Word Complete Flag in Status Register (SR) */
        s->regs[REG_INDEX(LPSPI_SR)] |= (1 << 8);
    }

    /* Set Frame Complete Flag in Status Register (SR) */
    s->regs[REG_INDEX(LPSPI_SR)] |= (1 << 9);

    /* Update interrupt status */
    s32k3x8_lpspi_update_irq(s);
}


static uint64_t s32k3x8_lpspi_read(void* opaque, hwaddr offset, unsigned size) {
    S32K3X8LPSPIState* s = opaque;
    uint32_t index = offset >> 2;
    uint32_t value = 0;

    if (offset >= S32K3X8_LPSPI_REG_SIZE) {
        qemu_log_mask(LOG_GUEST_ERROR,
            "[%s] %s: Invalid read offset 0x%" HWADDR_PRIx "\n",
            TYPE_S32K3X8_LPSPI, __func__, offset);
        return 0;
    }

    switch (offset) {
    case LPSPI_VERID:   // Explicitly implement VERID
    case LPSPI_PARAM:   // Explicitly implement PARAM
    case LPSPI_CR:
    case LPSPI_IER:
    case LPSPI_DER:
    case LPSPI_CFGR0:
    case LPSPI_CFGR1:
    case LPSPI_DMR0:
    case LPSPI_DMR1:
    case LPSPI_CCR:
    case LPSPI_CCR1:
    case LPSPI_FCR:
    case LPSPI_TCR:
    case LPSPI_SR:
    case LPSPI_RSR:     // Explicitly implement RSR
        value = s->regs[index];
        break;

    case LPSPI_FSR: // Detailed FIFO Status Register implementation
        value = ((fifo32_num_used(&s->rx_fifo) & 0x07) << 16) |
            (fifo32_num_used(&s->tx_fifo) & 0x07);
        break;

    case LPSPI_RDR:  // Read-only Receive Data Register
        if (!fifo32_is_empty(&s->rx_fifo)) {
            value = fifo32_pop(&s->rx_fifo);
        }
        else {
            value = 0xFFFFFFFF;
            s->regs[REG_INDEX(LPSPI_SR)] |= (1 << 12); // RX FIFO Underflow Flag
        }
        break;

    case LPSPI_TDR:  // Write-only register
    case LPSPI_TCBR: // Write-only register
        qemu_log_mask(LOG_GUEST_ERROR,
            "[%s] Illegal read from write-only offset 0x%" HWADDR_PRIx "\n",
            TYPE_S32K3X8_LPSPI, offset);
        value = 0;
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "[%s] Unhandled read offset 0x%" HWADDR_PRIx "\n",
            TYPE_S32K3X8_LPSPI, offset);
        break;
    }

    s32k3x8_lpspi_update_irq(s);
    return value;
}

static void s32k3x8_lpspi_write(void* opaque, hwaddr offset, uint64_t value, unsigned size) {

    //TCR is used as a static configuration register only.
    //Command configuration does NOT enter FIFO.
    //TDR writes directly enter FIFO as data only.

    S32K3X8LPSPIState* s = opaque;
    uint32_t index = offset >> 2;

    if (offset >= S32K3X8_LPSPI_REG_SIZE) {
        qemu_log_mask(LOG_GUEST_ERROR,
            "[%s] %s: Invalid write offset 0x%" HWADDR_PRIx "\n",
            TYPE_S32K3X8_LPSPI, __func__, offset);
        return;
    }

    switch (offset) {
    case LPSPI_CR:
        s->regs[index] = value;
        if (value & (1 << 1)) { // RST bit
            s32k3x8_lpspi_reset(DEVICE(s));
        }
        break;

    case LPSPI_SR:
        s->regs[index] &= ~value; // W1C behavior
        break;

    case LPSPI_IER:
    case LPSPI_DER:
    case LPSPI_CFGR0:
    case LPSPI_CFGR1:
    case LPSPI_DMR0:
    case LPSPI_DMR1:
    case LPSPI_CCR:
    case LPSPI_CCR1:
    case LPSPI_FCR:
    case LPSPI_TCR:
        // Simply update the command configuration register explicitly
        s->regs[index] = value;
        break;
    case LPSPI_TCBR:
        s->regs[index] = value;
        break;

    case LPSPI_TDR: // Explicitly handle write-only register
        if (!fifo32_is_full(&s->tx_fifo)) {
            fifo32_push(&s->tx_fifo, (uint32_t)value);
            s32k3x8_lpspi_handle_transfer(s);
        }
        else {
            s->regs[REG_INDEX(LPSPI_SR)] |= (1 << 11); // TX overflow error
        }
        break;

    case LPSPI_VERID: // Read-only registers
    case LPSPI_PARAM:
    case LPSPI_FSR:
    case LPSPI_RSR:
    case LPSPI_RDR:
        qemu_log_mask(LOG_GUEST_ERROR,
            "[%s] Illegal write to read-only offset 0x%" HWADDR_PRIx "\n",
            TYPE_S32K3X8_LPSPI, offset);
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "[%s] Unhandled write offset 0x%" HWADDR_PRIx "\n",
            TYPE_S32K3X8_LPSPI, offset);
        break;
    }

    s32k3x8_lpspi_update_irq(s);
}




static const MemoryRegionOps s32k3x8_lpspi_ops = {
    .read = s32k3x8_lpspi_read,
    .write = s32k3x8_lpspi_write,
    .valid.min_access_size = 1,
    .valid.max_access_size = 4,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription s32k3x8_lpspi_vmstate = {
    .name = TYPE_S32K3X8_LPSPI,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_FIFO32(tx_fifo, S32K3X8LPSPIState),
        VMSTATE_FIFO32(rx_fifo, S32K3X8LPSPIState),
        VMSTATE_UINT32_ARRAY(regs, S32K3X8LPSPIState, S32K3X8_LPSPI_NUM_REGS),
        VMSTATE_END_OF_LIST()
    }
};

static void s32k3x8_lpspi_realize(DeviceState* dev, Error** errp)
{
    S32K3X8LPSPIState* s = S32K3X8_LPSPI(dev);
    int i;

    memory_region_init_io(&s->iomem, OBJECT(s), &s32k3x8_lpspi_ops, s,
        TYPE_S32K3X8_LPSPI, S32K3X8_LPSPI_REG_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);
    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq);

    s->bus = ssi_create_bus(dev, "spi");

    for (i = 0; i < S32K3X8_LPSPI_CS_LINES; i++) {
        sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->cs_lines[i]);
    }

    fifo32_create(&s->tx_fifo, S32K3X8_LPSPI_FIFO_SIZE);
    fifo32_create(&s->rx_fifo, S32K3X8_LPSPI_FIFO_SIZE);
}

static void s32k3x8_lpspi_class_init(ObjectClass* klass, void* data)
{
    DeviceClass* dc = DEVICE_CLASS(klass);
    ResettableClass* rc = RESETTABLE_CLASS(klass);

    rc->phases.hold = s32k3x8_lpspi_reset;
    dc->vmsd = &s32k3x8_lpspi_vmstate;
    dc->realize = s32k3x8_lpspi_realize;
    dc->desc = "NXP S32K3X8 LPSPI Controller";
}

static const TypeInfo s32k3x8_lpspi_type_info = {
    .name = TYPE_S32K3X8_LPSPI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S32K3X8LPSPIState),
    .class_init = s32k3x8_lpspi_class_init,
};

static void s32k3x8_lpspi_register_types(void)
{
    type_register_static(&s32k3x8_lpspi_type_info);
}

type_init(s32k3x8_lpspi_register_types)
