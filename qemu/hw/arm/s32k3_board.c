#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "exec/memory.h"
#include "exec/address-spaces.h"
#include "sysemu/sysemu.h"
#include "hw/qdev-core.h"
#include "hw/sysbus.h"
#include "hw/arm/s32k3x8evb.h"
#include "hw/char/s32k3x8_uart.h"
#include "hw/ssi/s32k358_spi.h"
#include "hw/arm/boot.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-clock.h"
#include "hw/clock.h"
#include "qemu/units.h"
#include "migration/vmstate.h"
#include "hw/irq.h"
#include <stdio.h>

// Memory region definitions - Based on S32K358 linker file
#define INT_ITCM_BASE           0x00000000  // Instruction Tightly Coupled Memory
#define INT_ITCM_SIZE           (64 * KiB)  // 64KB

#define INT_DTCM_BASE           0x20000000  // Data Tightly Coupled Memory
#define INT_DTCM_SIZE           (124 * KiB) // 124KB
#define INT_STACK_DTCM_BASE     0x2001F000  // Stack area in DTCM
#define INT_STACK_DTCM_SIZE     (4 * KiB)   // 4KB

/*flash --774 page*/
#define FLASH_SIZE              0x00822000
#define INT_CODE_FLASH0_BASE    0x00400000
#define INT_CODE_FLASH0_SIZE    0x00200000    // 2 MB 
#define INT_CODE_FLASH1_BASE    0x00600000 
#define INT_CODE_FLASH1_SIZE    0x00200000    // 2 MB 
#define INT_CODE_FLASH2_BASE    0x00800000
#define INT_CODE_FLASH2_SIZE    0x00200000    // 2 MB 
#define INT_CODE_FLASH3_BASE    0x00A00000
#define INT_CODE_FLASH3_SIZE    0x00200000    // 2 MB 
#define INT_DATA_FLASH_BASE     0x10000000
#define INT_DATA_FLASH_SIZE     0x00020000    // 128 KB 
#define INT_UTEST_NVM_FLASH_BASE 0x1B000000
#define INT_UTEST_NVM_FLASH_SIZE 0x00002000    // 8 KB 


/*sram -- 762 page*/
#define SRAM_SIZE               0xC0000
#define INT_SRAM_STANDBY_BASE   0x20400000  // SRAM_standby, SPLIT FROM SRAM0
#define INT_SRAM_STANDBY_SIZE   0x10000     // 64KB
#define INT_SRAM_0_BASE         0x20410000
#define INT_SRAM_0_SIZE         0x30000     // 256KB-64KB = 192KB
#define INT_SRAM_1_BASE         0x20440000
#define INT_SRAM_1_SIZE         0x40000     // 256KB
#define INT_SRAM_2_BASE         0x20480000
#define INT_SRAM_2_SIZE         0x40000     // 256KB


// Peripheral base address definitions
#define S32K3_PERIPH_BASE        0x40000000

//lpuart instance addresses
#define S32K3_UART_BASE        0x40328000      // memory mapping file provides
#define S32K3_LPUART1_BASE     0x4032C000
#define S32K3_LPUART2_BASE     0x40330000
#define S32K3_LPUART3_BASE     0x40334000
#define S32K3_LPUART4_BASE     0x40338000
#define S32K3_LPUART5_BASE     0x4033C000
#define S32K3_LPUART6_BASE     0x40340000
#define S32K3_LPUART7_BASE     0x40344000
#define S32K3_LPUART8_BASE     0x4048C000
#define S32K3_LPUART9_BASE     0x40490000
#define S32K3_LPUART10_BASE    0x40494000
#define S32K3_LPUART11_BASE    0x40498000
#define S32K3_LPUART12_BASE    0x4049C000
#define S32K3_LPUART13_BASE    0x404A0000
#define S32K3_LPUART14_BASE    0x404A4000
#define S32K3_LPUART15_BASE    0x404A8000
// System frequency definitions
#define S32K3_SYSCLK_FREQ        (160 * 1000 * 1000)  // 160MHz

#define S32K3_LPSPI0_BASE        (S32K3_PERIPH_BASE + 0x358000) // 0x40358000
#define S32K3_LPSPI1_BASE        (S32K3_PERIPH_BASE + 0x35C000) // 0x4035C000
#define S32K3_LPSPI2_BASE        (S32K3_PERIPH_BASE + 0x360000) // 0x40360000
#define S32K3_LPSPI3_BASE        (S32K3_PERIPH_BASE + 0x364000) // 0x40364000
#define S32K3_LPSPI4_BASE        (S32K3_PERIPH_BASE + 0x4BC000) // 0x404BC000
#define S32K3_LPSPI5_BASE        (S32K3_PERIPH_BASE + 0x4C0000) // 0x404C0000

//LPSPI interrupt number definitions
#define S32K3_LPSPI0_IRQ         69
#define S32K3_LPSPI1_IRQ         70
#define S32K3_LPSPI2_IRQ         71
#define S32K3_LPSPI3_IRQ         72
#define S32K3_LPSPI4_IRQ         73
#define S32K3_LPSPI5_IRQ         74

/*Memory mapping initialization function*/
static void s32k3x8_initialize_memory_regions(MemoryRegion *system_memory)
{
    qemu_log_mask(CPU_LOG_INT, "\n------------------ Initialization of the memory regions ------------------\n");
    
    /* Allocate all memory regions */
    MemoryRegion *itcm = g_new(MemoryRegion, 1);
    MemoryRegion *dtcm = g_new(MemoryRegion, 1);
    MemoryRegion *dtcm_stack = g_new(MemoryRegion, 1);

    
    MemoryRegion *C0flash = g_new(MemoryRegion, 1);
    MemoryRegion *C1flash = g_new(MemoryRegion, 1);
    MemoryRegion *C2flash = g_new(MemoryRegion, 1);
    MemoryRegion *C3flash = g_new(MemoryRegion, 1);
    MemoryRegion *Dflash = g_new(MemoryRegion, 1);
    MemoryRegion *UNVMflash = g_new(MemoryRegion, 1);

    MemoryRegion *sram_standby = g_new(MemoryRegion, 1);
    MemoryRegion *sram0 = g_new(MemoryRegion, 1);
    MemoryRegion *sram1 = g_new(MemoryRegion, 1);
    MemoryRegion *sram2 = g_new(MemoryRegion, 1);
      
    /* ITCM init - RAM */
    qemu_log_mask(CPU_LOG_INT, "Initializing ITCM...\n");
    memory_region_init_ram(itcm, NULL, "s32k3x8.itcm", INT_ITCM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_ITCM_BASE, itcm);
    
    /* DTCM init - RAM */
    qemu_log_mask(CPU_LOG_INT, "Initializing DTCM...\n");
    memory_region_init_ram(dtcm, NULL, "s32k3x8.dtcm", INT_DTCM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_DTCM_BASE, dtcm);
    
    /* DTCM stack region */
    memory_region_init_ram(dtcm_stack, NULL, "s32k3x8.dtcm_stack", INT_STACK_DTCM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_STACK_DTCM_BASE, dtcm_stack);
    
    /* Program Flash initial - ROM */
    qemu_log_mask(CPU_LOG_INT, "Initializing Program Flash...\n");
    memory_region_init_rom(C0flash, NULL, "s32k3x8.C0flash", INT_CODE_FLASH0_SIZE, &error_fatal);
    memory_region_init_rom(C1flash, NULL, "s32k3x8.C1lash", INT_CODE_FLASH1_SIZE, &error_fatal);
    memory_region_init_rom(C2flash, NULL, "s32k3x8.C2lash", INT_CODE_FLASH2_SIZE, &error_fatal);
    memory_region_init_rom(C3flash, NULL, "s32k3x8.C3lash", INT_CODE_FLASH3_SIZE, &error_fatal);
    memory_region_init_rom(Dflash, NULL, "s32k3x8.Dflash", INT_DATA_FLASH_SIZE, &error_fatal);
    memory_region_init_rom(UNVMflash, NULL, "s32k3x8.UNVMflash", INT_UTEST_NVM_FLASH_SIZE, &error_fatal);

    memory_region_add_subregion(system_memory, INT_CODE_FLASH0_BASE, C0flash);
    memory_region_add_subregion(system_memory, INT_CODE_FLASH1_BASE, C1flash);
    memory_region_add_subregion(system_memory, INT_CODE_FLASH2_BASE, C2flash);
    memory_region_add_subregion(system_memory, INT_CODE_FLASH3_BASE, C3flash);
    memory_region_add_subregion(system_memory, INT_DATA_FLASH_BASE, Dflash);
    memory_region_add_subregion(system_memory, INT_UTEST_NVM_FLASH_BASE, UNVMflash);
    
    /* SRAM region init */
    qemu_log_mask(CPU_LOG_INT, "Initializing SRAM regions...\n");
    memory_region_init_ram(sram_standby, NULL, "s32k3x8.sram_standby", INT_SRAM_STANDBY_SIZE, &error_fatal);
    memory_region_init_ram(sram0, NULL, "s32k3x8.sram0", INT_SRAM_0_SIZE, &error_fatal);
    memory_region_init_ram(sram1, NULL, "s32k3x8.sram1", INT_SRAM_1_SIZE, &error_fatal);
    memory_region_init_ram(sram2, NULL, "s32k3x8.sram2", INT_SRAM_2_SIZE, &error_fatal);

    memory_region_add_subregion(system_memory, INT_SRAM_STANDBY_BASE, sram_standby);
    memory_region_add_subregion(system_memory, INT_SRAM_0_BASE, sram0);
    memory_region_add_subregion(system_memory, INT_SRAM_1_BASE, sram1);
    memory_region_add_subregion(system_memory, INT_SRAM_2_BASE, sram2);
    qemu_log_mask(CPU_LOG_INT, "Memory regions initialized successfully.\n");

  

}
static void s32k3x8_init_lpspi(S32K3X8EVBState *s, MemoryRegion *system_memory, 
                               Clock *sysclk, ARMv7MState *armv7m)
{
    DeviceState *dev;
    
    qemu_log_mask(CPU_LOG_INT, "Initializing LPSPI devices\n");
    
    dev = qdev_new(TYPE_S32K3X8_LPSPI);
    s->lpspi[0] = dev;
    if (!sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_abort)) {
        error_report("Failed to realize LPSPI0");
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, S32K3_LPSPI0_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, 
                       qdev_get_gpio_in(DEVICE(armv7m), S32K3_LPSPI0_IRQ));
    
    dev = qdev_new(TYPE_S32K3X8_LPSPI);
    s->lpspi[1] = dev;
    if (!sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_abort)) {
        error_report("Failed to realize LPSPI1");
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, S32K3_LPSPI1_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, 
                       qdev_get_gpio_in(DEVICE(armv7m), S32K3_LPSPI1_IRQ));
    
    dev = qdev_new(TYPE_S32K3X8_LPSPI);
    s->lpspi[2] = dev;
    if (!sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_abort)) {
        error_report("Failed to realize LPSPI2");
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, S32K3_LPSPI2_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, 
                       qdev_get_gpio_in(DEVICE(armv7m), S32K3_LPSPI2_IRQ));
    
    dev = qdev_new(TYPE_S32K3X8_LPSPI);
    s->lpspi[3] = dev;
    if (!sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_abort)) {
        error_report("Failed to realize LPSPI3");
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, S32K3_LPSPI3_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, 
                       qdev_get_gpio_in(DEVICE(armv7m), S32K3_LPSPI3_IRQ));
    
    dev = qdev_new(TYPE_S32K3X8_LPSPI);
    s->lpspi[4] = dev;
    if (!sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_abort)) {
        error_report("Failed to realize LPSPI4");
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, S32K3_LPSPI4_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, 
                       qdev_get_gpio_in(DEVICE(armv7m), S32K3_LPSPI4_IRQ));
    
    dev = qdev_new(TYPE_S32K3X8_LPSPI);
    s->lpspi[5] = dev;
    if (!sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_abort)) {
        error_report("Failed to realize LPSPI5");
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, S32K3_LPSPI5_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, 
                       qdev_get_gpio_in(DEVICE(armv7m), S32K3_LPSPI5_IRQ));
    
    qemu_log_mask(CPU_LOG_INT, "LPSPI devices initialized successfully\n");
}


/*board_init*/
static void s32k3x8evb_init(MachineState *machine)
{
    S32K3X8EVBState *s = S32K3X8EVB_MACHINE(machine);
    Error *error_local = NULL;
    DeviceState *dev;
    
    qemu_log_mask(CPU_LOG_INT, "Initializing S32K3X8EVB board\n");
    
    /*1. Check and get system memory*/
    MemoryRegion *system_memory = get_system_memory();
    if (!system_memory) {
        error_report("Failed to get system memory");
        return;
    }
    
    /*2. Use memory mapping initialization method*/
    s32k3x8_initialize_memory_regions(system_memory);
    
    /*3. Initialize system clock*/
    Clock *sysclk = clock_new(OBJECT(machine), "SYSCLK");
    if (!sysclk) {
        error_report("Failed to create system clock");
        return;
    }
    clock_set_hz(sysclk, S32K3_SYSCLK_FREQ);
    
  
    /*4. Initialize ARM core*/
    object_initialize_child(OBJECT(machine), "armv7m", &s->armv7m, TYPE_ARMV7M);
    
    /*5. Configure CPU*/
    qdev_prop_set_string(DEVICE(&s->armv7m), "cpu-type", ARM_CPU_TYPE_NAME("cortex-m7"));
    qdev_prop_set_uint32(DEVICE(&s->armv7m), "init-svtor", INT_ITCM_BASE); // Vector table at address 0 (ITCM)
    qdev_prop_set_uint8(DEVICE(&s->armv7m), "num-prio-bits", 4);  // Cortex-M7 uses 4 priority bits
    qdev_prop_set_uint32(DEVICE(&s->armv7m), "num-irq", 240);     // Number of interrupts for S32K3X8
    
   
    /*6. Set up system connections*/
    object_property_set_link(OBJECT(&s->armv7m), "memory", 
                           OBJECT(system_memory), &error_abort);
    qdev_connect_clock_in(DEVICE(&s->armv7m), "cpuclk", sysclk);
    
    /* 7. Implement system bus device*/
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->armv7m), &error_local)) {
        error_reportf_err(error_local, "Failed to realize ARM core: ");
        return;
    }
     /* 8. Load firmware*/
    armv7m_load_kernel(ARM_CPU(first_cpu), machine->kernel_filename, INT_CODE_FLASH0_BASE, FLASH_SIZE);

    /*9. Initialize UART*/
    qemu_log_mask(CPU_LOG_INT, "Initializing UART\n");
    dev = qdev_new(TYPE_S32E8_LPUART);
    if (!dev) {
        error_report("Failed to create UART device");
        return;
    }
    
    /*10. Configure UART*/
    qdev_prop_set_chr(dev, "chardev", serial_hd(0));
    s->uart = dev;
    
    if (!sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_local)) {
        error_reportf_err(error_local, "Failed to realize UART: ");
        return;
    }
    
    /*11. Map UART - adjusted according to reference manual*/
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, S32K3_UART_BASE);
  
    /*12. Initialize LPSPI devices*/
    s32k3x8_init_lpspi(s, system_memory, sysclk, &s->armv7m);    
    qemu_log_mask(CPU_LOG_INT, "S32K3X8EVB board initialization complete\n"); 


}

/*Board class initialization*/
static void s32k3x8evb_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    mc->desc = "NXP S32K3X8EVB Development Board (Cortex-M7)";
    mc->init = s32k3x8evb_init;
    mc->default_cpus = 1;
    mc->min_cpus = 1;
    mc->max_cpus = 2;
    mc->default_ram_size = SRAM_SIZE;
}

static const TypeInfo s32k3x8evb_type = {
    .name = MACHINE_TYPE_NAME("s32k3x8evb"),
    .parent = TYPE_MACHINE,
    .instance_size = sizeof(S32K3X8EVBState),
    .class_init = s32k3x8evb_class_init,
};

/* Register machine type*/
static void s32k3x8evb_machine_init(void)
{
    qemu_log_mask(CPU_LOG_INT, "Registering S32K3X8EVB machine type\n");
    type_register_static(&s32k3x8evb_type);
}

type_init(s32k3x8evb_machine_init)
