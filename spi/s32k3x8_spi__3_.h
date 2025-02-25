
#ifndef HW_S32K3X8_SPI_H
#define HW_S32K3X8_SPI_H

#include "hw/sysbus.h"
#include "hw/ssi/ssi.h"
#include "qom/object.h"

/* Define SPI register offsets */
#define S32K3X8_SPI_CR1     0x00
#define S32K3X8_SPI_CR2     0x04
#define S32K3X8_SPI_SR      0x08
#define S32K3X8_SPI_DR      0x0C
#define S32K3X8_SPI_CRCPR   0x10
#define S32K3X8_SPI_RXCRCR  0x14
#define S32K3X8_SPI_TXCRCR  0x18

/* Define SPI control and status bits */
#define S32K3X8_SPI_CR1_SPE  (1 << 6)
#define S32K3X8_SPI_CR1_MSTR (1 << 2)
#define S32K3X8_SPI_SR_RXNE  (1 << 0)

#define TYPE_S32K3X8_SPI "s32k3x8-spi"
OBJECT_DECLARE_SIMPLE_TYPE(S32K3X8SPIState, S32K3X8_SPI)

struct S32K3X8SPIState {
    SysBusDevice parent_obj; /* Base object */

    MemoryRegion mmio;       /* Memory-mapped I/O region */

    uint32_t spi_cr1;        /* Control register 1 */
    uint32_t spi_cr2;        /* Control register 2 */
    uint32_t spi_sr;         /* Status register */
    uint32_t spi_dr;         /* Data register */
    uint32_t spi_crcpr;      /* CRC polynomial register */
    uint32_t spi_rxcrcr;     /* RX CRC register */
    uint32_t spi_txcrcr;     /* TX CRC register */

    qemu_irq irq;            /* Interrupt line */
    SSIBus* ssi;             /* SPI bus */
};

#endif /* HW_S32K3X8_SPI_H */