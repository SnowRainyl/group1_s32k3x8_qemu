#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h" 
/* UART register definitions */
#define S32K3_PERIPH_BASE        0x40000000
#define S32K3_UART_BASE          (S32K3_PERIPH_BASE + 0x4A000) 

/* UART register offsets */
#define GLOBAL_OFFSET     0x08    /* Global register */
#define BAUD_OFFSET       0x10    /* Baud rate register */
#define STAT_OFFSET       0x14    /* Status register */
#define CTRL_OFFSET       0x18    /* Control register */
#define DATA_OFFSET       0x1C    /* Data register */
#define FIFO_OFFSET       0x28    /* FIFO register */
#define WATER_OFFSET      0x2C    /* Watermark register */

/* UART bit definitions */
#define GLOBAL_RST        (1 << 0)    /* Software reset */
#define GLOBAL_ENABLE     (1 << 1)    /* UART enable */
#define CTRL_TE           (1 << 19)   /* Transmit enable */
#define CTRL_RE           (1 << 18)   /* Receive enable */
#define STAT_TC           (1 << 22)   /* Transmission complete flag */
#define STAT_TDRE         (1 << 23)   /* Transmit data register empty flag */
#define FIFO_RXFE         (1 << 3)    /* Receive FIFO enable */
#define FIFO_TXFE         (1 << 7)    /* Transmit FIFO enable */

/* Access macros */
#define UART_REG(offset) (*(volatile uint32_t *)(S32K3_UART_BASE + (offset)))
#define mainTASK_PRIORITY ( tskIDLE_PRIORITY + 2 )

/* UART driver functions */
static void uart_init(void)
{
    /* Reset UART */
    UART_REG(GLOBAL_OFFSET) |= GLOBAL_RST;
    
    /* Enable UART after reset */
    UART_REG(GLOBAL_OFFSET) |= GLOBAL_ENABLE;
    
    /* Enable FIFOs */
    UART_REG(FIFO_OFFSET) |= (FIFO_RXFE | FIFO_TXFE);
    
    /* Configure watermark - set to 0 to trigger status changes as soon as possible */
    UART_REG(WATER_OFFSET) = 0x00000000;
    
    /* Enable transmitter and receiver */
    UART_REG(CTRL_OFFSET) |= (CTRL_TE | CTRL_RE);
}

static void uart_send_byte(uint8_t byte)
{
    /* Wait until transmit data register is empty */
    while(!(UART_REG(STAT_OFFSET) & STAT_TDRE));
    
    /* Send byte */
    UART_REG(DATA_OFFSET) = byte;
    
    /* Note: No need to check TC because we use TDRE to control the flow */
}

static void uart_send_string(const char *str)
{
    while (*str) {
        uart_send_byte((uint8_t)(*str));
        str++;
    }
    
    /* Wait for all data to be transmitted */
    while(!(UART_REG(STAT_OFFSET) & STAT_TC));
}

/* FreeRTOS task */
void TxTask(void *parameters)
{
    (void)parameters;
    
    /* Task loop */
    for(;;) {
        uart_send_string("Hello from FreeRTOS running in QEMU on S32K3X8!\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));  /* Delay for 1 second */
    }
}

/* Main function */
int main(void)
{
    /* Initialize UART - only once */
    uart_init();
    uart_send_string(">>> S32K3X8 FreeRTOS Demo started\r\n");
    uart_send_string(">>> Initializing...\r\n");

    /* Create transmit task */
    if (xTaskCreate(TxTask, "TxTask", configMINIMAL_STACK_SIZE, NULL, mainTASK_PRIORITY, NULL) != pdPASS) {
        uart_send_string("ERROR: Failed to create TxTask\r\n");
        return 1;
    }
    
    uart_send_string(">>> Starting FreeRTOS scheduler...\r\n");
    
    /* Start FreeRTOS scheduler */
    vTaskStartScheduler();

    /* If the scheduler returns, an error has occurred */
    uart_send_string("ERROR: Scheduler returned (should never happen)\r\n");
    
    return 1;
}
