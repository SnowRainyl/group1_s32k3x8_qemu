#include <stdint.h>
#include <stddef.h>    
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "LPUART.h" 
/* Queue variable declarations */
static QueueHandle_t system_uart_rx_queue = NULL;
static QueueHandle_t system_uart_tx_queue = NULL;
static volatile int uart_initialized = 0;

typedef void (*pFunc)(void);

/* External declarations */
extern void __PROGRAM_START(void);
extern void SystemInit(void);
extern uint32_t __stack;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __etext;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __HeapLimit;
extern uint32_t __StackLimit;
extern int main(void);

/* FreeRTOS handler functions */
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

extern void uart_init(void);


/* Function declarations */
void Reset_Handler(void) __attribute__((naked, noreturn));
void Default_Handler(void);

/* System exception handlers */
void NMI_Handler(void)         __attribute__ ((weak, alias("Default_Handler")));
void HardFault_Handler(void)   __attribute__ ((weak, alias("Default_Handler")));
void MemManage_Handler(void)   __attribute__ ((weak, alias("Default_Handler")));
void BusFault_Handler(void)    __attribute__ ((weak, alias("Default_Handler")));
void UsageFault_Handler(void)  __attribute__ ((weak, alias("Default_Handler")));
void DebugMon_Handler(void)    __attribute__ ((weak, alias("Default_Handler")));

/* IRQ handlers */
void INT0_Handler(void)        __attribute__ ((weak, alias("Default_Handler")));
void INT1_Handler(void)        __attribute__ ((weak, alias("Default_Handler")));
void LPUART_Handler(void);


/* Vector table */
__attribute__ ((section(".isr_vector")))
const pFunc __Vectors[] = {
    (pFunc) &__stack,             /* Initial Stack Pointer */
    Reset_Handler,                /* Reset Handler */
    NMI_Handler,                  /* NMI Handler */
    HardFault_Handler,            /* Hard Fault Handler */
    MemManage_Handler,            /* MemManage Handler */
    BusFault_Handler,             /* BusFault Handler */
    UsageFault_Handler,           /* UsageFault Handler */
    0, 0, 0, 0,                   /* Reserved */
    vPortSVCHandler,              /* SVCall Handler */
    DebugMon_Handler,             /* Debug Monitor Handler */
    0,                            /* Reserved */
    xPortPendSVHandler,           /* PendSV Handler */
    xPortSysTickHandler,          /* SysTick Handler */

    // External interrupts
    INT0_Handler,
    INT1_Handler,
    LPUART_Handler,               /* UART Interrupt Handler */
};

/* UART Initialization Function */
void uart_init(void)
{
    if (uart_initialized) {
        return;  /* Avoid re-initialization */
    }

    /* Hardware initialization */
    UART_REG(GLOBAL_OFFSET) |= GLOBAL_RST;
    UART_REG(GLOBAL_OFFSET) |= GLOBAL_ENABLE;
    UART_REG(FIFO_OFFSET) |= (FIFO_RXFE | FIFO_TXFE); // Assuming these are FIFO flush bits
    UART_REG(WATER_OFFSET) = 0x00000000; // Watermark level
    UART_REG(CTRL_OFFSET) |= (CTRL_TE | CTRL_RE); // Enable Transmitter and Receiver

    /* Create queues */
    system_uart_rx_queue = xQueueCreate(32, sizeof(uint8_t));
    system_uart_tx_queue = xQueueCreate(64, sizeof(uint8_t));

    /* Enable receive interrupt */
    UART_REG(CTRL_OFFSET) |= CTRL_RIE; // Receive Interrupt Enable

    uart_initialized = 1;
}

/* System-level UART send string interface */
int system_uart_send_string(const char *str)
{
    if (str == NULL || system_uart_tx_queue == NULL) {
        return -1; // Invalid argument or queue not initialized
    }

    /* Put string into TX queue */
    while (*str) {
        if (xQueueSend(system_uart_tx_queue, str, pdMS_TO_TICKS(100)) != pdTRUE) {
            return -2;  /* Queue full or timeout */
        }
        str++;
    }

    /* Enable transmit interrupt to start sending */
    UART_REG(CTRL_OFFSET) |= CTRL_TIE; // Transmit Interrupt Enable

    return 0; // Success
}

/* System-level UART receive byte interface */
int system_uart_receive_byte(uint8_t *byte, uint32_t timeout_ms)
{
    if (byte == NULL || system_uart_rx_queue == NULL) {
        return -1; // Invalid argument or queue not initialized
    }

    if (xQueueReceive(system_uart_rx_queue, byte, pdMS_TO_TICKS(timeout_ms)) == pdTRUE) {
        return 0;   /* Successfully received */
    }

    return -2;  /* Timeout */
}

/* Get UART status register */
uint32_t system_uart_get_status(void)
{
    return UART_REG(STAT_OFFSET);
}

/* Reset Handler Function */
void Reset_Handler(void)
{
    uint32_t *src, *dst;

    /* Copy data from Flash to RAM (.data section) */
    src = &__etext;        // End of text section (start of data in Flash)
    dst = &__data_start__; // Start of data section in RAM
    while (dst < &__data_end__) {
        *dst++ = *src++;
    }

    /* Zero-initialize the .bss section */
    dst = &__bss_start__;
    while (dst < &__bss_end__) {
        *dst++ = 0;
    }

    /* Initialize system (e.g., clock, peripherals) */
    SystemInit();

    /* Call main function */
    main();

    while (1); /* Infinite loop / Safety net */
}

/* Default Interrupt Handler */
void Default_Handler(void)
{
    while (1);
}

/* UART Interrupt Handler - Implementation */
void LPUART_Handler(void)
{
    uint32_t status = UART_REG(STAT_OFFSET);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* Handle receive interrupt (Receive Data Register Full) */
    if (status & STAT_RDRF) {
        uint8_t received_byte = (uint8_t)(UART_REG(DATA_OFFSET) & 0xFF);

        /* Put received data into system RX queue */
        if (system_uart_rx_queue != NULL) {
            xQueueSendFromISR(system_uart_rx_queue, &received_byte, &xHigherPriorityTaskWoken);
        }
        // Note: Clearing RDRF flag might be done by reading DATA_OFFSET or writing to a status/clear register,
        // depending on the UART peripheral. Ensure this is handled correctly by hardware or explicitly.
    }

    /* Handle transmit interrupt (Transmit Data Register Empty) */
    if (status & STAT_TDRE) {
        uint8_t tx_byte;

        if (system_uart_tx_queue != NULL &&
            xQueueReceiveFromISR(system_uart_tx_queue, &tx_byte, &xHigherPriorityTaskWoken) == pdTRUE) {
            /* Send next byte */
            UART_REG(DATA_OFFSET) = tx_byte;
        } else {
            /* TX Queue empty, disable transmit interrupt */
            UART_REG(CTRL_OFFSET) &= ~CTRL_TIE;
        }
        // Note: TDRE flag is usually cleared by writing to DATA_OFFSET.
    }

    /* If a task switch is needed due to ISR activity, perform it */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
