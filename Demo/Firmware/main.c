#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h" 
#include "LPUART.h"
#include "LPSPI.h"    
#include <stdio.h>      
#include <string.h>    

void uart_send_byte(uint8_t byte)
{
    /* Wait until transmit data register is empty */
    while(!(UART_REG(STAT_OFFSET) & STAT_TDRE));
    
    /* Send byte */
    UART_REG(DATA_OFFSET) = byte;
    
    /* Note: No need to check TC because we use TDRE to control the flow */
}

void uart_send_string(const char *str)
{
    while (*str) {
        uart_send_byte((uint8_t)(*str));
        str++;
    }
    
    /* Wait for all data to be transmitted */
    while(!(UART_REG(STAT_OFFSET) & STAT_TC));
}

/* FreeRTOS task */
void TxTask1(void *parameters)
{
    (void)parameters;
    
    /* Task loop */
    for(;;) {
        uart_send_string("|======================================================|\r\n");
        uart_send_string(" Hello task1 from FreeRTOS running in QEMU on S32K3X8!\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));  
    }
}

void TxTask2(void *parameters)
{
    (void)parameters;
    
    /* Task loop */
    for(;;) {
        uart_send_string("|======================================================|\r\n");
        uart_send_string(" Hello task2 from FreeRTOS running in QEMU on S32K3X8!\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));  
    }
}

/* UART register status check task */
void UartStatusTask(void *parameters)
{
    (void)parameters;
    uint32_t check_count = 0;
    
    uart_send_string(">>> UART Status Check Task Started\r\n");
    
    for(;;) {
        check_count++;
        
        uart_send_string("|======================================================|\r\n");
        uart_send_string(" UART Status Check #");
        
        char status_str[32];
        sprintf(status_str, "%lu", check_count);
        uart_send_string(status_str);
        uart_send_string("\r\n");
        
        /* Read and display the UART status register*/
        uint32_t status_reg = UART_REG(STAT_OFFSET);
        sprintf(status_str, " STAT Register: 0x%08lX\r\n", status_reg);
        uart_send_string(status_str);
        
        /* Check each status bit */
        uart_send_string(" Status Bits:\r\n");
        uart_send_string(status_reg & STAT_TDRE ? "  TDRE: SET (TX Ready)\r\n" : "  TDRE: CLEAR\r\n");
        uart_send_string(status_reg & STAT_TC ? "  TC: SET (TX Complete)\r\n" : "  TC: CLEAR\r\n");
        uart_send_string(status_reg & STAT_RDRF ? "  RDRF: SET (RX Data Available)\r\n" : "  RDRF: CLEAR\r\n");
        
        /* Check FIFO status*/
        uint32_t fifo_reg = UART_REG(FIFO_OFFSET);
        sprintf(status_str, " FIFO Register: 0x%08lX\r\n", fifo_reg);
        uart_send_string(status_str);
        
        vTaskDelay(pdMS_TO_TICKS(3000)); 
    }
}

/* Added: SPI test task */
void SpiTestTask(void *parameters)
{
    (void)parameters;
    uint32_t test_count = 0;
    
    uart_send_string(">>> SPI Test Task Started\r\n");
    
    /* Initialize SPI */
    lpspi_init();
    uart_send_string(">>> SPI Initialized\r\n");
    
    for(;;) {
        test_count++;
        
        uart_send_string("\r\n");
        uart_send_string("+======================================================+\r\n");
        uart_send_string("| SPI TEST CYCLE #");
        
        char test_str[32];
        sprintf(test_str, "%lu", test_count);
        uart_send_string(test_str);
        uart_send_string(" |\r\n");
        uart_send_string("+======================================================+\r\n");
        
        /* 1. SPI status check */
        uart_send_string("\r\n--- SPI Status Check ---\r\n");
        lpspi_status_check();
        
        /* 2. SPI loopback test */
        uart_send_string("\r\n--- SPI Loopback Test ---\r\n");
        lpspi_loopback_test();
        
        /* 3. Single byte transfer test */
        uart_send_string("\r\n--- Single Byte Transfer Test ---\r\n");
        
        uint8_t test_values[] = {0x00, 0xFF, 0xAA, 0x55, 0x12, 0x34};
        for (int i = 0; i < 6; i++) {
            uint8_t tx_data = test_values[i];
            uint8_t rx_data = 0;
            
            if (lpspi_transfer_byte(tx_data, &rx_data)) {
                sprintf(test_str, "TX: 0x%02X -> RX: 0x%02X %s\r\n", 
                       tx_data, rx_data, 
                       (tx_data == rx_data) ? "PASS" : "FAIL");
                uart_send_string(test_str);
            } else {
                sprintf(test_str, "TX: 0x%02X -> Transfer FAILED\r\n", tx_data);
                uart_send_string(test_str);
            }
        }
        
        uart_send_string("\r\n--- Test Cycle Complete ---\r\n");
        
        /* Wait longer before next test cycle */
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* Added: SPI continuous test task */
void SpiContinuousTask(void *parameters)
{
    (void)parameters;
    uint32_t transfer_count = 0;
    
    uart_send_string(">>> SPI Continuous Test Task Started\r\n");
    
    /* Wait for SPI initialization to complete */
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    for(;;) {
        transfer_count++;
        
        /* Send incremental data pattern */
        uint8_t tx_data = (uint8_t)(transfer_count & 0xFF);
        uint8_t rx_data = 0;
        
        if (lpspi_transfer_byte(tx_data, &rx_data)) {
            /* Output status every 100 transfers */
            if (transfer_count % 100 == 0) {
                char continuous_str[64];
                sprintf(continuous_str, "[SPI Continuous] Count: %lu, Last TX/RX: 0x%02X/0x%02X\r\n",
                       transfer_count, tx_data, rx_data);
                uart_send_string(continuous_str);
            }
        } else {
            char error_str[64];
            sprintf(error_str, "[SPI Error] Transfer %lu failed\r\n", transfer_count);
            uart_send_string(error_str);
        }
        
        /* Short delay */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


void SpiRegisterTestTask(void *parameters)
{
    (void)parameters;
    uint32_t test_count = 0;
    
    uart_send_string(">>> SPI Register Test Task Started\r\n");
    
    for(;;) {
        test_count++;
        
        uart_send_string("|======================================================|\r\n");
        uart_send_string(" SPI Register Test #");
        
        char test_str[32];
        sprintf(test_str, "%lu", test_count);
        uart_send_string(test_str);
        uart_send_string("\r\n");
        
        /* Try to read SPI registers */
        uart_send_string(" Testing SPI register access...\r\n");
        
        /* Read VERID register - this register should be read-only, safer */
        uint32_t verid = LPSPI_REG(VERID_OFFSET);
        sprintf(test_str, " VERID: 0x%08lX\r\n", verid);
        uart_send_string(test_str);
        
        /* Read PARAM register */
        uint32_t param = LPSPI_REG(PARAM_OFFSET);
        sprintf(test_str, " PARAM: 0x%08lX\r\n", param);
        uart_send_string(test_str);
        
        /* Read status register */
        uint32_t sr = LPSPI_REG(SR_OFFSET);
        sprintf(test_str, " SR: 0x%08lX\r\n", sr);
        uart_send_string(test_str);
        
        uart_send_string(" Register access test completed\r\n");
        
        vTaskDelay(pdMS_TO_TICKS(3000)); 
    }
}


/* Main function */
int main(void)
{
    /* Initialize UART - only once */
    uart_init();
    uart_send_string(">>> S32K3X8 FreeRTOS + SPI Demo started\r\n");
    uart_send_string(">>> Initializing...\r\n");

    /* Create UART tasks */
    if (xTaskCreate(TxTask1, "TxTask1", configMINIMAL_STACK_SIZE, NULL, mainTASK_PRIORITY, NULL) != pdPASS) {
        uart_send_string("ERROR: Failed to create TxTask1\r\n");
        return 1;
    }
    
    if(xTaskCreate(TxTask2, "TxTask2", configMINIMAL_STACK_SIZE, NULL, mainTASK_PRIORITY-1, NULL) != pdPASS) {
        uart_send_string("ERROR: Failed to create TxTask2\r\n");
        return 1;
    }
    
    if(xTaskCreate(UartStatusTask, "StatusTask", configMINIMAL_STACK_SIZE, NULL, mainTASK_PRIORITY-2, NULL) != pdPASS) {
        uart_send_string("ERROR: Failed to create StatusTask\r\n");
        return 1;
    }
    
    /* Added: Create SPI test tasks */
   
    if(xTaskCreate(SpiTestTask, "SpiTestTask", configMINIMAL_STACK_SIZE * 2, NULL, mainTASK_PRIORITY-2, NULL) != pdPASS) {
        uart_send_string("ERROR: Failed to create SpiTestTask\r\n");
        return 1;
    }
    
    if(xTaskCreate(SpiContinuousTask, "SpiContTask", configMINIMAL_STACK_SIZE, NULL, mainTASK_PRIORITY-2, NULL) != pdPASS) {
        uart_send_string("ERROR: Failed to create SpiContinuousTask\r\n");
        return 1;
    }
    

    
    uart_send_string(">>> All tasks created successfully\r\n");
    uart_send_string(">>> Starting FreeRTOS scheduler...\r\n");
    
    /* Start FreeRTOS scheduler */
    vTaskStartScheduler();

    /* If the scheduler returns, an error has occurred */
    uart_send_string("ERROR: Scheduler returned (should never happen)\r\n");
    
    return 1;
}
