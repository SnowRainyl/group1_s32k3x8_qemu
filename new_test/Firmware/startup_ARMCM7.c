#include<stdint.h>

typedef void (*pFunc)(void);
/**/
/* External declaration of the entry point and system functions */
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

/*extern freertos handler functions*/
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

void Reset_Handler(void) __attribute__((naked, noreturn));
void Default_Handler(void);

/* Weak definitions for core exception handlers */
void NMI_Handler(void)              __attribute__ ((weak, alias("Default_Handler")));
void HardFault_Handler(void)        __attribute__ ((weak, alias("Default_Handler")));
void MemManage_Handler(void)        __attribute__ ((weak, alias("Default_Handler")));
void BusFault_Handler(void)         __attribute__ ((weak, alias("Default_Handler")));
void UsageFault_Handler(void)       __attribute__ ((weak, alias("Default_Handler")));
void DebugMon_Handler(void)         __attribute__ ((weak, alias("Default_Handler")));

/* IRQ handlers */
void INT0_Handler(void)             __attribute__ ((weak, alias("Default_Handler")));
void INT1_Handler(void)             __attribute__ ((weak, alias("Default_Handler")));
/* ... more as needed */

/* Vector table */
__attribute__ ((section(".isr_vector")))
const pFunc __Vectors[] = {
  (pFunc) &__stack,       /* Initial Stack Pointer */
  Reset_Handler,               /* Reset Handler */
  NMI_Handler,                 /* NMI Handler */
  HardFault_Handler,           /* Hard Fault Handler */
  MemManage_Handler,           /* MemManage Handler */
  BusFault_Handler,            /* BusFault Handler */
  UsageFault_Handler,          /* UsageFault Handler */
  0, 0, 0, 0,                  /* Reserved */
  vPortSVCHandler,                 /* change to freertos SVCall Handler */
  DebugMon_Handler,            /* Debug Monitor Handler */
  0,                           /* Reserved */
  xPortPendSVHandler,              /* change to freertos PendSV Handler */
  xPortSysTickHandler,             /* change to freertos SysTick Handler */

  // Interrupts
  INT0_Handler,
  INT1_Handler,
  
};

/* Reset handler: Initialize memory and jump to main */
void Reset_Handler(void)
{
  uint32_t *src, *dst;

  /* Copy data from Flash to RAM */
  src = &__etext;
  dst = &__data_start__;
  while (dst < &__data_end__) {
    *dst++ = *src++;
  }

  /* Zero-initialize the .bss section */
  dst = &__bss_start__;
  while (dst < &__bss_end__) {
    *dst++ = 0;
  }

  

  /* Init system */
  SystemInit();

  /* calls main */
  main();

  while (1); // just in case
}

/* Default Handler for unused IRQs */
void Default_Handler(void)
{
  while (1);
}
