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
       //	uart_send_string("|------------------------------------------------------|\r\n");
        uart_send_string(" Hello task1 from FreeRTOS running in QEMU on S32K3X8!\r\n");
       //	uart_send_string("|------------------------------------------------------|\r\n");

        vTaskDelay(pdMS_TO_TICKS(3000));  
    }
}

void TxTask2(void *parameters)
{
    (void)parameters;
    
    /* Task loop */
    for(;;) {
       	//uart_send_string("|------------------------------------------------------|\r\n");
        uart_send_string(" Hello task2 from FreeRTOS running in QEMU on S32K3X8!\r\n");
       	//uart_send_string("|------------------------------------------------------|\r\n");


        vTaskDelay(pdMS_TO_TICKS(3000));  
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
        uart_send_string("|------F UART status check!----------------------------|\r\n");
        vTaskDelay(pdMS_TO_TICKS(10000)); 
    }
}

void SimpleSpiTestTask(void *parameters)
{
    (void)parameters;
    uint32_t test_count = 0;
    uart_send_string(">>> Simple SPI Test Started\r\n");
    uint32_t verid = LPSPI_REG(VERID_OFFSET);
    uint32_t param = LPSPI_REG(PARAM_OFFSET);
    uint32_t sr = LPSPI_REG(SR_OFFSET);
    
    char reg_str[64];
    sprintf(reg_str, "VERID: 0x%08lX, PARAM: 0x%08lX, SR: 0x%08lX\r\n", 
            verid, param, sr);
    uart_send_string(reg_str);
    
    for(;;) {
        test_count++;
        
        uart_send_string("|======================================================|\r\n");
        char test_str[32];
        sprintf(test_str, "| SPI TEST #%lu |\r\n", test_count);
        uart_send_string(test_str);
         uart_send_string("|======================================================|\r\n");
        
        lpspi_status_check();
        lpspi_loopback_test();
        
      
        uart_send_string("|------F SPI test !------------------------------------|\r\n");
         
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* Main function */
int main(void)
{
    /* Initialize UART and SPI */
    uart_init();
    lpspi_init();

    uart_send_string(">>> S32K3X8 FreeRTOS Demo started\r\n");
    uart_send_string(">>> Initializing...\r\n");

    /* Create UART tasks */
    if (xTaskCreate(TxTask1, "TxTask1", configMINIMAL_STACK_SIZE, NULL, mainTASK_PRIORITY -2, NULL) != pdPASS) {
        uart_send_string("ERROR: Failed to create TxTask1\r\n");
        return 1;
    }
    
    if(xTaskCreate(TxTask2, "TxTask2", configMINIMAL_STACK_SIZE, NULL,  mainTASK_PRIORITY -1, NULL) != pdPASS) {
        uart_send_string("ERROR: Failed to create TxTask2\r\n");
        return 1;
    }
    
    if(xTaskCreate(UartStatusTask, "StatusTask", configMINIMAL_STACK_SIZE, NULL,  mainTASK_PRIORITY -3, NULL) != pdPASS) {
        uart_send_string("ERROR: Failed to create LpuartStatusTask\r\n");
        return 1;
    }
    
    /* Added: Create SPI test tasks */
   
    if(xTaskCreate(SimpleSpiTestTask, "SpiTestTask", configMINIMAL_STACK_SIZE * 2, NULL,  mainTASK_PRIORITY -4, NULL) != pdPASS) {
        uart_send_string("ERROR: Failed to create SpiTestTask\r\n");
        return 1;
    }
    
    uart_send_string(">>> All tasks created successfully\r\n");
    uart_send_string(">>> Starting scheduler...\r\n");
    
    /* Start FreeRTOS scheduler */
    vTaskStartScheduler();

    /* If the scheduler returns, an error has occurred */
    uart_send_string("ERROR: Scheduler returned (should never happen)\r\n");
    
    return 1;
}
