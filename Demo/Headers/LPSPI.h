#ifndef LPSPI_H
#define LPSPI_H
#include <stdint.h>
#include <stdbool.h>
/* LPSPI base address - based on S32K3X8 reference manual */
#define LPSPI0_BASE_ADDR    0x40358000
#define LPSPI1_BASE_ADDR    0x4035C000
#define LPSPI2_BASE_ADDR    0x40360000
#define LPSPI3_BASE_ADDR    0x40364000
/* Use LPSPI0 as default test interface */
#define LPSPI_BASE          LPSPI0_BASE_ADDR
/* LPSPI register offset addresses */
#define VERID_OFFSET        0x00    /* Version ID Register */
#define PARAM_OFFSET        0x04    /* Parameter Register */
#define CR_OFFSET           0x10    /* Control Register */
#define SR_OFFSET           0x14    /* Status Register */
#define IER_OFFSET          0x18    /* Interrupt Enable Register */
#define DER_OFFSET          0x1C    /* DMA Enable Register */
#define CFGR0_OFFSET        0x20    /* Configuration 0 Register */
#define CFGR1_OFFSET        0x24    /* Configuration 1 Register */
#define DMR0_OFFSET         0x30    /* Data Match 0 Register */
#define DMR1_OFFSET         0x34    /* Data Match 1 Register */
#define CCR_OFFSET          0x40    /* Clock Configuration Register */
#define CCR1_OFFSET         0x44    /* Clock Configuration 1 Register */
#define FCR_OFFSET          0x58    /* FIFO Control Register */
#define FSR_OFFSET          0x5C    /* FIFO Status Register */
#define TCR_OFFSET          0x60    /* Transmit Command Register */
#define TDR_OFFSET          0x64    /* Transmit Data Register */
#define RSR_OFFSET          0x70    /* Receive Status Register */
#define RDR_OFFSET          0x74    /* Receive Data Register */
/* Register access macro */
#define LPSPI_REG(offset)   (*(volatile uint32_t *)((LPSPI_BASE) + (offset)))
/* Control Register (CR) bit definitions */
#define CR_MEN              (1 << 0)    /* Module Enable */
#define CR_RST              (1 << 1)    /* Software Reset */
#define CR_DBGEN            (1 << 3)    /* Debug Enable */
#define CR_RTF              (1 << 8)    /* Reset Transmit FIFO */
#define CR_RRF              (1 << 9)    /* Reset Receive FIFO */
/* Status Register (SR) bit definitions */
#define SR_TDF              (1 << 0)    /* Transmit Data Flag */
#define SR_RDF              (1 << 1)    /* Receive Data Flag */
#define SR_WCF              (1 << 8)    /* Word Complete Flag */
#define SR_FCF              (1 << 9)    /* Frame Complete Flag */
#define SR_TCF              (1 << 10)   /* Transfer Complete Flag */
#define SR_TEF              (1 << 11)   /* Transmit Error Flag */
#define SR_REF              (1 << 12)   /* Receive Error Flag */
#define SR_MBF              (1 << 24)   /* Module Busy Flag */
/* Configuration 1 Register (CFGR1) bit definitions */
#define CFGR1_MASTER        (1 << 0)    /* Controller Mode */
#define CFGR1_SAMPLE        (1 << 1)    /* Sample Point */
#define CFGR1_AUTOPCS       (1 << 2)    /* Automatic PCS */
#define CFGR1_NOSTALL       (1 << 3)    /* No Stall */
/* Transmit Command Register (TCR) bit definitions */
#define TCR_FRAMESZ_MASK    0x00000FFF  /* Frame Size Mask (8-bit data = 7) */
#define TCR_WIDTH_1BIT      (0 << 16)   /* 1-bit transfer */
#define TCR_PCS0            (0 << 24)   /* Use PCS[0] */
#define TCR_PRESCALE_1      (0 << 27)   /* Divide by 1 */
#define TCR_CPOL_LOW        (0 << 31)   /* Clock idle low */
#define TCR_CPHA_FIRST      (0 << 30)   /* Capture on first edge */
/* FIFO Control Register (FCR) bit definitions */
#define FCR_TXWATER_MASK    0x0000000F  /* TX Watermark */
#define FCR_RXWATER_MASK    0x000F0000  /* RX Watermark */
/* Function declarations */
void lpspi_init(void);
bool lpspi_transmit_byte(uint8_t data);
bool lpspi_receive_byte(uint8_t *data);
bool lpspi_transfer_byte(uint8_t tx_data, uint8_t *rx_data);
void lpspi_reset_fifos(void);
uint32_t lpspi_get_status(void);
bool lpspi_is_busy(void);
/* Test functions */
void lpspi_loopback_test(void);
void lpspi_status_check(void);
#endif /* LPSPI_H */
