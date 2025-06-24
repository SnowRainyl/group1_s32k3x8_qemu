#ifndef S32K3X8_LPSPI_H
#define S32K3X8_LPSPI_H

#include "hw/ssi/ssi.h"
#include "hw/sysbus.h"
#include "qemu/fifo32.h"
#include "qom/object.h"

/** Size of register I/O address space used by LPSPI device */
#define S32K3X8_LPSPI_REG_SIZE (0x1000)

/** Total number of known registers (assumed based on register space) */
#define S32K3X8_LPSPI_NUM_REGS    (S32K3X8_LPSPI_REG_SIZE / sizeof(uint32_t))
#define S32K3X8_LPSPI_FIFO_SIZE   (4)  /* 4x32-bit FIFO for both transmit and receive data */
#define S32K3X8_LPSPI_CS_LINES    (4)  /* Number of chip-select lines */

#define TYPE_S32K3X8_LPSPI        "s32k3x8.lpspi"
OBJECT_DECLARE_SIMPLE_TYPE(S32K3X8LPSPIState, S32K3X8_LPSPI)

struct S32K3X8LPSPIState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    MemoryRegion iomem;
    SSIBus* bus;
    qemu_irq irq;
    qemu_irq cs_lines[S32K3X8_LPSPI_CS_LINES];

    uint32_t regs[S32K3X8_LPSPI_NUM_REGS];

    Fifo32 rx_fifo;
    Fifo32 tx_fifo;
};

#endif /* S32K3X8_LPSPI_H */
