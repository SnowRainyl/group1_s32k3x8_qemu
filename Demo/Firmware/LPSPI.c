#include "LPSPI.h"
#include "LPUART.h"  /* For debug output */
#include <stdio.h>
#include <string.h>

/* Delay function - simple busy wait */
static void lpspi_delay_us(uint32_t us)
{
    /* Simple delay implementation, assuming system clock is 160MHz */
    volatile uint32_t count = us * 40;  /* About 160/4 = 40 cycles per us */
    while (count--) {
        __asm volatile("nop");
    }
}

/* Wait for status bit to be set, with timeout */
static bool lpspi_wait_flag(uint32_t flag, bool set, uint32_t timeout_us)
{
    uint32_t count = 0;
    while (count < timeout_us) {
        uint32_t status = LPSPI_REG(SR_OFFSET);
        if (set) {
            if (status & flag) return true;
        } else {
            if (!(status & flag)) return true;
        }
        lpspi_delay_us(1);
        count++;
    }
    return false;
}

/* LPSPI initialization */
void lpspi_init(void)
{
    /* 1. Software reset */
    LPSPI_REG(CR_OFFSET) = CR_RST;
    lpspi_delay_us(10);
    
    /* 2. Clear reset bit */
    LPSPI_REG(CR_OFFSET) = 0;
    lpspi_delay_us(10);
    
    /* 3. Configure as master mode */
    LPSPI_REG(CFGR1_OFFSET) = CFGR1_MASTER;
    
    /* 4. Set clock configuration - simple configuration */
    LPSPI_REG(CCR_OFFSET) = 0x00000000;    /* Minimum delay */
    LPSPI_REG(CCR1_OFFSET) = 0x00000000;   /* Minimum delay */
    
    /* 5. Set transmit command - 8-bit data, 1-bit transfer */
    uint32_t tcr = 7;  /* 8-bit data (FRAMESZ = 7) */
    tcr |= TCR_WIDTH_1BIT;
    tcr |= TCR_PCS0;
    tcr |= TCR_PRESCALE_1;
    tcr |= TCR_CPOL_LOW;
    tcr |= TCR_CPHA_FIRST;
    LPSPI_REG(TCR_OFFSET) = tcr;
    
    /* 6. Set FIFO watermark */
    LPSPI_REG(FCR_OFFSET) = 0x00000000;    /* TX and RX watermark both 0 */
    
    /* 7. Reset FIFO */
    lpspi_reset_fifos();
    
    /* 8. Enable module */
    LPSPI_REG(CR_OFFSET) = CR_MEN;
    
    /* 9. Wait for module initialization complete */
    lpspi_delay_us(100);
}

/* Reset FIFO */
void lpspi_reset_fifos(void)
{
    uint32_t cr = LPSPI_REG(CR_OFFSET);
    LPSPI_REG(CR_OFFSET) = cr | CR_RTF | CR_RRF;
    lpspi_delay_us(10);
    LPSPI_REG(CR_OFFSET) = cr;
}

/* Transmit one byte */
bool lpspi_transmit_byte(uint8_t data)
{
    /* Wait for TX FIFO not full */
    if (!lpspi_wait_flag(SR_TDF, true, 1000)) {
        return false;
    }
    
    /* Send data */
    LPSPI_REG(TDR_OFFSET) = data;
    
    /* Wait for transfer complete */
    if (!lpspi_wait_flag(SR_TCF, true, 1000)) {
        return false;
    }
    
    /* Clear transfer complete flag */
    LPSPI_REG(SR_OFFSET) = SR_TCF;
    
    return true;
}

/* Receive one byte */
bool lpspi_receive_byte(uint8_t *data)
{
    if (data == NULL) return false;
    
    /* Wait for RX data available */
    if (!lpspi_wait_flag(SR_RDF, true, 1000)) {
        return false;
    }
    
    /* Read data */
    *data = (uint8_t)(LPSPI_REG(RDR_OFFSET) & 0xFF);
    
    return true;
}

/* Bidirectional transfer of one byte */
bool lpspi_transfer_byte(uint8_t tx_data, uint8_t *rx_data)
{
    /* Wait for TX FIFO not full */
    if (!lpspi_wait_flag(SR_TDF, true, 1000)) {
        return false;
    }
    
    /* Send data */
    LPSPI_REG(TDR_OFFSET) = tx_data;
    
    /* Wait for receive data */
    if (!lpspi_wait_flag(SR_RDF, true, 1000)) {
        return false;
    }
    
    /* Read received data */
    if (rx_data) {
        *rx_data = (uint8_t)(LPSPI_REG(RDR_OFFSET) & 0xFF);
    }
    
    /* Wait for transfer complete */
    if (!lpspi_wait_flag(SR_TCF, true, 1000)) {
        return false;
    }
    
    /* Clear transfer complete flag */
    LPSPI_REG(SR_OFFSET) = SR_TCF;
    
    return true;
}

/* Get status register */
uint32_t lpspi_get_status(void)
{
    return LPSPI_REG(SR_OFFSET);
}

/* Check if busy */
bool lpspi_is_busy(void)
{
    return (LPSPI_REG(SR_OFFSET) & SR_MBF) != 0;
}

/* SPI loopback test */
void lpspi_loopback_test(void)
{
    static const uint8_t test_data[] = {0xAA, 0x55, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    const uint32_t test_count = sizeof(test_data);
    uint32_t success_count = 0;
    uint32_t error_count = 0;
    char debug_str[128];
    
    /* Send debug info */
    uart_send_string("=== SPI Loopback Test Start ===\r\n");
    
    for (uint32_t i = 0; i < test_count; i++) {
        uint8_t tx_data = test_data[i];
        uint8_t rx_data = 0;
        
        sprintf(debug_str, "Test %lu: TX=0x%02X ", i + 1, tx_data);
        uart_send_string(debug_str);
        
        /* Execute transfer */
        if (lpspi_transfer_byte(tx_data, &rx_data)) {
            sprintf(debug_str, "RX=0x%02X ", rx_data);
            uart_send_string(debug_str);
            
            /* In loopback mode, received data should equal transmitted data */
            if (rx_data == tx_data) {
                uart_send_string("PASS\r\n");
                success_count++;
            } else {
                uart_send_string("FAIL (Data Mismatch)\r\n");
                error_count++;
            }
        } else {
            uart_send_string("FAIL (Transfer Error)\r\n");
            error_count++;
        }
        
        /* Small delay */
        lpspi_delay_us(1000);
    }
    
    /* Test result statistics */
    sprintf(debug_str, "=== Test Results: %lu PASS, %lu FAIL ===\r\n", 
            success_count, error_count);
    uart_send_string(debug_str);
}

/* SPI status check */
void lpspi_status_check(void)
{
    char debug_str[128];
    
    /* Read various registers */
    uint32_t verid = LPSPI_REG(VERID_OFFSET);
    uint32_t param = LPSPI_REG(PARAM_OFFSET);
    uint32_t cr = LPSPI_REG(CR_OFFSET);
    uint32_t sr = LPSPI_REG(SR_OFFSET);
    uint32_t cfgr1 = LPSPI_REG(CFGR1_OFFSET);
    uint32_t tcr = LPSPI_REG(TCR_OFFSET);
    uint32_t fsr = LPSPI_REG(FSR_OFFSET);
    
    uart_send_string("=== SPI Status Check ===\r\n");
    
    sprintf(debug_str, "VERID:  0x%08lX\r\n", verid);
    uart_send_string(debug_str);
    
    sprintf(debug_str, "PARAM:  0x%08lX\r\n", param);
    uart_send_string(debug_str);
    
    sprintf(debug_str, "CR:     0x%08lX ", cr);
    uart_send_string(debug_str);
    uart_send_string(cr & CR_MEN ? "(ENABLED)" : "(DISABLED)");
    uart_send_string("\r\n");
    
    sprintf(debug_str, "SR:     0x%08lX\r\n", sr);
    uart_send_string(debug_str);
    
    /* Detailed status bit information */
    uart_send_string("Status Flags:\r\n");
    uart_send_string(sr & SR_TDF ? "  TDF: SET (TX Ready)\r\n" : "  TDF: CLEAR\r\n");
    uart_send_string(sr & SR_RDF ? "  RDF: SET (RX Ready)\r\n" : "  RDF: CLEAR\r\n");
    uart_send_string(sr & SR_WCF ? "  WCF: SET (Word Complete)\r\n" : "  WCF: CLEAR\r\n");
    uart_send_string(sr & SR_FCF ? "  FCF: SET (Frame Complete)\r\n" : "  FCF: CLEAR\r\n");
    uart_send_string(sr & SR_TCF ? "  TCF: SET (Transfer Complete)\r\n" : "  TCF: CLEAR\r\n");
    uart_send_string(sr & SR_MBF ? "  MBF: SET (Module Busy)\r\n" : "  MBF: CLEAR\r\n");
    
    sprintf(debug_str, "CFGR1:  0x%08lX ", cfgr1);
    uart_send_string(debug_str);
    uart_send_string(cfgr1 & CFGR1_MASTER ? "(MASTER)" : "(SLAVE)");
    uart_send_string("\r\n");
    
    sprintf(debug_str, "TCR:    0x%08lX\r\n", tcr);
    uart_send_string(debug_str);
    
    sprintf(debug_str, "FSR:    0x%08lX\r\n", fsr);
    uart_send_string(debug_str);
    
    uart_send_string("========================\r\n");
}
