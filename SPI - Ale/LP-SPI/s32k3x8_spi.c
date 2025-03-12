#include "s32k3x8_spi.h"
#include "hw/qdev-core.h"
#include "hw/sysbus.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/timer.h"

// Reset handler
static void s32k3x8_lpspi_reset(DeviceState *dev)
{
    S32K3X8LPSPIDevice *s = (S32K3X8LPSPIDevice *)dev;
    memset(s->regs, 0, sizeof(s->regs));
    s->tx_fifo_head = s->tx_fifo_tail = s->tx_fifo_count = 0;
    s->rx_fifo_head = s->rx_fifo_tail = s->rx_fifo_count = 0;
}

// FIFO helpers
static inline void lpspi_tx_fifo_push(S32K3X8LPSPIDevice *s, uint32_t data)
{
    if (s->tx_fifo_count < LPSPI_FIFO_DEPTH) {
        s->tx_fifo[s->tx_fifo_tail] = data;
        s->tx_fifo_tail = (s->tx_fifo_tail + 1) % LPSPI_FIFO_DEPTH;
        s->tx_fifo_count++;
    } else {
        qemu_log_mask(LOG_GUEST_ERROR, "LPSPI TX FIFO overflow\n");
    }
}

static inline uint32_t lpspi_rx_fifo_pop(S32K3X8LPSPIDevice *s)
{
    if (s->rx_fifo_count > 0) {
        uint32_t data = s->rx_fifo[s->rx_fifo_head];
        s->rx_fifo_head = (s->rx_fifo_head + 1) % LPSPI_FIFO_DEPTH;
        s->rx_fifo_count--;
        return data;
    } else {
        qemu_log_mask(LOG_GUEST_ERROR, "LPSPI RX FIFO underflow\n");
        return 0;
    }
}

// Read handler
static uint64_t s32k3x8_lpspi_read(void *opaque, hwaddr offset, unsigned size)
{
    S32K3X8LPSPIDevice *s = (S32K3X8LPSPIDevice *)opaque;
    offset >>= 2; // Convert to word index

    switch (offset) {
        case LPSPI_RDR >> 2:
            return lpspi_rx_fifo_pop(s);
        case LPSPI_SR >> 2:
            return (s->rx_fifo_count > 0) ? 0x02 : 0x00; // Simple flag logic
        default:
            return s->regs[offset];
    }
}

// Write handler
static void s32k3x8_lpspi_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    S32K3X8LPSPIDevice *s = (S32K3X8LPSPIDevice *)opaque;
    offset >>= 2;

    switch (offset) {
        case LPSPI_TDR >> 2:
            lpspi_tx_fifo_push(s, (uint32_t)value);
            break;
        case LPSPI_CR >> 2:
            if (value & 0x1) {
                qemu_log("LPSPI enabled\n");
            }
            break;
        default:
            s->regs[offset] = value;
    }
}

// Memory mapping
static const MemoryRegionOps s32k3x8_lpspi_ops = {
    .read = s32k3x8_lpspi_read,
    .write = s32k3x8_lpspi_write,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

// Device initialization
static void s32k3x8_lpspi_init(Object *obj)
{
    S32K3X8LPSPIDevice *s = (S32K3X8LPSPIDevice *)obj;
    memory_region_init_io(&s->iomem, obj, &s32k3x8_lpspi_ops, s, "s32k3x8-lpspi", LPSPI_REG_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->iomem);
}

// Type definition
static const TypeInfo s32k3x8_lpspi_info = {
    .name = "s32k3x8-lpspi",
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S32K3X8LPSPIDevice),
    .instance_init = s32k3x8_lpspi_init,
    .class_init = s32k3x8_lpspi_reset,
};

// Register device type
static void s32k3x8_lpspi_register_types(void)
{
    type_register_static(&s32k3x8_lpspi_info);
}
type_init(s32k3x8_lpspi_register_types);

