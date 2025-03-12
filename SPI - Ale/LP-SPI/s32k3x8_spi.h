#ifndef S32K3X8_SPI_H
#define S32K3X8_SPI_H

#include "hw/sysbus.h"
#include "qemu/timer.h"
#include "qemu/log.h"
#include "hw/qdev-properties.h"

#define LPSPI_REG_SIZE   0x1000  // Register block size
#define LPSPI_FIFO_DEPTH 4       // 4x32-bit FIFO size

// Register offsets
#define LPSPI_VERID  0x00  // Version ID
#define LPSPI_PARAM  0x04  // Parameter Register
#define LPSPI_CR     0x10  // Control Register
#define LPSPI_TCR    0x60  // Transmit Command Register
#define LPSPI_TDR    0x64  // Transmit Data Register
#define LPSPI_RSR    0x70  // Receive Status Register
#define LPSPI_RDR    0x74  // Receive Data Register
#define LPSPI_SR     0x14  // Status Register
#define LPSPI_IER    0x18  // Interrupt Enable
#define LPSPI_DER    0x1C  // DMA Enable

typedef struct {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    uint32_t regs[LPSPI_REG_SIZE / 4];  // Register storage
    uint32_t tx_fifo[LPSPI_FIFO_DEPTH]; // TX FIFO
    uint32_t rx_fifo[LPSPI_FIFO_DEPTH]; // RX FIFO
    int tx_fifo_head, tx_fifo_tail, tx_fifo_count;
    int rx_fifo_head, rx_fifo_tail, rx_fifo_count;
    QEMUTimer spi_timer;
} S32K3X8LPSPIDevice;

#endif // S32K3X8_SPI_H

