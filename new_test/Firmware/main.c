
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/* UART base address */
#define UART_BASE 0x40000000U

/* UART register offsets */
#define UART_GLOBAL_OFFSET   0x08
#define UART_CTRL_OFFSET     0x18
#define UART_DATA_OFFSET     0x1C

/* UART bit definitions */
#define GLOBAL_RST   (1 << 0)
#define GLOBAL_ENABLE (1 << 1)
#define CTRL_TE      (1 << 19)
#define CTRL_RE      (1 << 18)

/* Access macros */
#define UART_REG(offset) (*(volatile uint32_t *)(UART_BASE + (offset)))

/* UART driver functions */
static void uart_init(void)
{
    // Reset UART
    UART_REG(UART_GLOBAL_OFFSET) |= GLOBAL_RST;

    // Enable UART after reset
    UART_REG(UART_GLOBAL_OFFSET) |= GLOBAL_ENABLE;

    // Enable transmitter
    UART_REG(UART_CTRL_OFFSET) |= CTRL_TE;
}

static void uart_send_byte(uint8_t byte)
{
    UART_REG(UART_DATA_OFFSET) = byte;
}

static void uart_send_string(const char *str)
{
    while (*str) {
        uart_send_byte((uint8_t)(*str));
        str++;
    }
}

/* FreeRTOS task */
void TxTask(void *parameters)
{
    for(;;) {
        uart_send_string("Hello from FreeRTOS running in QEMU!\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay 1 second
    }
}

/* main function */
int main(void)
{
    uart_init();

    xTaskCreate(TxTask, "TxTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();

    // Should never reach here
    while (1) {}
}

