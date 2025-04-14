/*****************************************************************************
 *
 * Copyright 2022 NXP
 * All Rights Reserved
 *
 *****************************************************************************
 *
 * THIS SOFTWARE IS PROVIDED BY NXP "AS IS" AND ANY EXPRESSED OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL NXP OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

__STACK_SIZE = 0x00001000;
__HEAP_SIZE  = 0x00001000;

/* Linker script to configure memory regions. */
MEMORY
{
  /*queues and performance-critical data*/
  ITCM            (RWX) : ORIGIN = 0x00000000, LENGTH = 0x10000
  /*stack, heap, critical data*/
  DTCM             (RW) : ORIGIN = 0x20000000, LENGTH = 0x20000 

  M7_0_SRAM_STDBY  (RW) : ORIGIN = 0x20400000, LENGTH = 0x10000

  M7_0_PFLASH (RX)      : ORIGIN = 0x00400000, LENGTH = 0x200000
  M7_0_DFLASH (RX)      : ORIGIN = 0x10000000, LENGTH = 0xC000
  M7_0_SRAM   (RW)      : ORIGIN = 0x20410000, LENGTH = 0x30000
//enventhough the cortex m7 shows that 358 doesn't include core1 instance but in physical it include. because core1 work together with core0, it means them be seen as a logic unit due to lockstep mechanism .
//锁步模式是一种安全机制，其中两个核心执行完全相同的指令，然后比较结果以检测任何错误。从功能角度看，这被视为单个处理器核心（因为两个核心一起工作作为一个逻辑单元），这就是为什么实例表显示CM7_1为"No"。 
  M7_1_PFLASH (RX)      : ORIGIN = 0x00600000, LENGTH = 0x200000
  M7_1_DFLASH (RX)      : ORIGIN = 0x1000C000, LENGTH = 0xA000
  M7_1_SRAM   (RW)      : ORIGIN = 0x20440000, LENGTH = 0x40000
  
  M7_2_PFLASH (RX)      : ORIGIN = 0x00800000, LENGTH = 0x400000
  M7_2_DFLASH (RX)      : ORIGIN = 0x10016000, LENGTH = 0xA000
  M7_2_SRAM   (RW)      : ORIGIN = 0x20480000, LENGTH = 0x40000
}

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __ecc_table_start__
 *   __ecc_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */
ENTRY(Reset_Handler)

SECTIONS
{
  .vectors://begin at 0x0 address
  {
    KEEP(*(.vectors)) //'keep' keyword prevents compiler optimize this when them not cited by code.

  } > ITCM

  .text :
  {
    *(.text*)
    KEEP(*(.init))//usually generate by compiler, program init code
    KEEP(*(.fini))//program finish init code

    /* .ctors */
    /*place the constructor table in crtbegin.o*/
    *crtbegin.o(.ctors)// If our code doesn't include that exception code, the compiler will handle this perfectly, so putting them in here is fine.
    *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    /* .dtors */
     *crtbegin.o(.dtors)
     *crtbegin?.o(.dtors)
     *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
     *(SORT(.dtors.*))
     *(.dtors)

    *(.rodata*)
    KEEP(*(.eh_frame*))//exception handling frame
  } > M7_0_PFLASH  //puts entire .text segment into M7_0PFLASH memory region

  .ARM.extab :  // If our code doesn't include that exception code, the compiler will handle this perfectly, so putting them in here is fine.
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > M7_0_PFLASH  //Puts those segments one by one in order, due to they decide to put multiple segments to the same memory region M7_0_PFLASH

  __exidx_start = .;
  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } > M7_0_PFLASH
  __exidx_end = .;

  .ecc.table :
  {
    . = ALIGN(4);
    __ecc_table_start__ = .;

    QUAD (__data_start__)
    QUAD ((__data_end__ - __data_start__) / 8)

    QUAD (__bss_start__)
    QUAD ((__bss_end__ - __bss_start__) / 8)

    QUAD (__HeapTop)
    QUAD ((__HeapLimit - __HeapTop) / 8)

    QUAD (__StackLimit)
    QUAD ((__StackTop - __StackLimit) / 8)

    QUAD (ORIGIN(ITCM))
    QUAD (LENGTH(ITCM) / 8)

    QUAD (ORIGIN(DTCM))
    QUAD (LENGTH(DTCM) / 8)

    __ecc_table_end__ = .;
  } > M7_0_PFLASH

  .copy.table :
  {
    . = ALIGN(4);
    __copy_table_start__ = .;

    LONG (__etext)
    LONG (__data_start__)
    LONG ((__data_end__ - __data_start__) / 4)

    /* Add each additional data section here */
    /*
      LONG (__etext2)
      LONG (__data2_start__)
      LONG ((__data2_end__ - __data2_start__) / 4)
    */
    __copy_table_end__ = .;
  } > M7_0_PFLASH

  .zero.table :
  {
    . = ALIGN(4);
    __zero_table_start__ = .;
    /* Add each additional bss section here */
    /*
      LONG (__bss2_start__)
      LONG ((__bss2_end__ - __bss2_start__) / 4)
    */
    __zero_table_end__ = .;
  } > M7_0_PFLASH

  __etext = ALIGN(8);

  .boot_header :
  {
    KEEP(*(.boot_header))
  } > M7_0_DFLASH

  .standby_ram :
  {
    *(.standby_ram)
  } > M7_0_SRAM_STDBY

  /* Due ECC initialization sequence __data_start__ and __data_end__ should be aligned on 8 bytes */
  .data : AT (__etext)
  {
    . = ALIGN(8);
    __data_start__ = .;
    *(vtable)
    *(.data)
    *(.data.*)

    . = ALIGN(4);
    /* preinit data */
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);

    . = ALIGN(4);
    /* init data */
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);

    . = ALIGN(4);
    /* finit data */
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN (__fini_array_end = .);

    KEEP(*(.jcr*))
    . = ALIGN(8);
    /* All data end */
    __data_end__ = .;

  } > M7_0_SRAM

  /* Due ECC initialization sequence __bss_start__ and __bss_end__ should be aligned on 8 bytes */
  .bss :
  {
    . = ALIGN(8);
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(8);
    __bss_end__ = .;
  } > M7_0_SRAM

  /* Due ECC initialization sequence __HeapTop and __HeapLimit should be aligned on 8 bytes */
  .heap (COPY):
  {
    . = ALIGN(8);
    __HeapTop = .;
    __end__ = .;
    _end = .;
    PROVIDE(end = .);
    . = . + __HEAP_SIZE;
    . = ALIGN(8);
    __HeapLimit = .;
  } > M7_0_SRAM

  /* Due ECC initialization sequence __StackLimit and __StackTop should be aligned on 8 bytes */
  .stack (ORIGIN(M7_0_SRAM) + LENGTH(M7_0_SRAM) - __STACK_SIZE) (COPY) :
  {
    . = ALIGN(8);//Rounds up to the next address divisible by 8
    __StackLimit = .;//This line assigns the value of the current position to the symbol __StackLimit, which marks the starting position of the stack space (i.e., the lower boundary of the stack).
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    __StackTop = .;
  } > M7_0_SRAM
  PROVIDE(__stack = __StackTop);

  /* Check if data + heap + stack exceeds RAM limit */
  ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
  CM7_0_START_ADDRESS = ORIGIN(M7_0_PFLASH);
  CM7_1_START_ADDRESS = ORIGIN(M7_1_PFLASH);
  CM7_2_START_ADDRESS = ORIGIN(M7_2_PFLASH);
  __STDBYRAM_START = ORIGIN(M7_0_SRAM_STDBY);
  __STDBYRAM_SIZE = LENGTH(M7_0_SRAM_STDBY);
}