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
#include "hw/arm/boot.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-clock.h"
#include "hw/clock.h"
#include "qemu/units.h"
#include "migration/vmstate.h"
#include "hw/irq.h"
#include <stdio.h>

// 内存区域定义 - 基于S32K358的链接文件
#define INT_ITCM_BASE           0x00000000  // 指令紧耦合内存
#define INT_ITCM_SIZE           (64 * KiB)  // 64KB

#define INT_DTCM_BASE           0x20000000  // 数据紧耦合内存
#define INT_DTCM_SIZE           (124 * KiB) // 124KB
#define INT_STACK_DTCM_BASE     0x2001F000  // DTCM中的栈区域
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


// 外设基地址定义
#define S32K3_PERIPH_BASE        0x40000000
#define S32K3_UART_BASE          (S32K3_PERIPH_BASE + 0x4A000) // 根据S32K3参考手册调整

// 系统频率定义
#define S32K3_SYSCLK_FREQ        (160 * 1000 * 1000)  // 160MHz


static void fake_systick_tick(void *opaque)
{
    S32K3X8EVBState *s = (S32K3X8EVBState *)opaque;

    // Pulse SysTick interrupt
    qemu_irq_pulse(qdev_get_gpio_in(DEVICE(&s->armv7m), 15));  // SysTick is interrupt #15

    // Re-arm the timer to fire again after 1ms
    timer_mod(s->systick_timer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) + 1);
}
// 内存映射初始化函数
static void s32k3x8_initialize_memory_regions(MemoryRegion *system_memory)
{
    qemu_log_mask(CPU_LOG_INT, "\n------------------ Initialization of the memory regions ------------------\n");
    
    /* 分配所有内存区域 */
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

    /*alias*/
    MemoryRegion *flash_alias = g_new(MemoryRegion, 1);
    memory_region_init_alias(flash_alias, NULL, "s32k3x8.flash_alias", 
                         C0flash, 0, INT_ITCM_SIZE);
    memory_region_add_subregion_overlap(system_memory, 0, flash_alias, 1); 

}

// board_init

static void s32k3x8evb_init(MachineState *machine)
{
    S32K3X8EVBState *s = S32K3X8EVB_MACHINE(machine);
    Error *error_local = NULL;
    DeviceState *dev;
    
    qemu_log_mask(CPU_LOG_INT, "Initializing S32K3X8EVB board\n");
    
    // 1. 检查并获取系统内存
    MemoryRegion *system_memory = get_system_memory();
    if (!system_memory) {
        error_report("Failed to get system memory");
        return;
    }
    
    // 2. 使用内存映射初始化方法
    s32k3x8_initialize_memory_regions(system_memory);
    
    // 3. 初始化系统时钟
    Clock *sysclk = clock_new(OBJECT(machine), "SYSCLK");
    if (!sysclk) {
        error_report("Failed to create system clock");
        return;
    }
    clock_set_hz(sysclk, S32K3_SYSCLK_FREQ);
    
    // 4. no alias pflash-->itcm    
   /* 创建别名映射: FLASH映射到地址0 */
    // 6. 初始化ARM核心
    object_initialize_child(OBJECT(machine), "armv7m", &s->armv7m, TYPE_ARMV7M);
    
    // 配置CPU
    qdev_prop_set_string(DEVICE(&s->armv7m), "cpu-type", ARM_CPU_TYPE_NAME("cortex-m7"));
    qdev_prop_set_uint32(DEVICE(&s->armv7m), "init-svtor", 0); // 向量表在地址0 (ITCM)
    qdev_prop_set_uint8(DEVICE(&s->armv7m), "num-prio-bits", 4);  // Cortex-M7 使用4位优先级
    qdev_prop_set_uint32(DEVICE(&s->armv7m), "num-irq", 240);     // S32K3X8的中断数量
    
   
    // 7. 设置系统连接
    object_property_set_link(OBJECT(&s->armv7m), "memory", 
                           OBJECT(system_memory), &error_abort);
    qdev_connect_clock_in(DEVICE(&s->armv7m), "cpuclk", sysclk);
    
    // 8. 实现系统总线设备
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->armv7m), &error_local)) {
        error_reportf_err(error_local, "Failed to realize ARM core: ");
        return;
    }
     // 5. 加载固件
    armv7m_load_kernel(ARM_CPU(first_cpu), machine->kernel_filename, INT_CODE_FLASH0_BASE, FLASH_SIZE);

    // 9. 初始化UART
    qemu_log_mask(CPU_LOG_INT, "Initializing UART\n");
    dev = qdev_new(TYPE_S32E8_LPUART);
    if (!dev) {
        error_report("Failed to create UART device");
        return;
    }
    
    // 配置UART
    qdev_prop_set_chr(dev, "chardev", serial_hd(0));
    s->uart = dev;
    
    if (!sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_local)) {
        error_reportf_err(error_local, "Failed to realize UART: ");
        return;
    }
    
    // 映射UART - 按照参考手册调整
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, S32K3_UART_BASE);
    qemu_log_mask(CPU_LOG_INT, "S32K3X8EVB board initialization complete\n");       

}

// 板级类初始化
static void s32k3x8evb_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    mc->desc = "NXP S32K3X8EVB Development Board (Cortex-M7)";
    mc->init = s32k3x8evb_init;
    mc->default_cpus = 1;
    mc->min_cpus = 1;
    mc->max_cpus = 1;
    mc->default_ram_size = SRAM_SIZE;
}

static const TypeInfo s32k3x8evb_type = {
    .name = MACHINE_TYPE_NAME("s32k3x8evb"),
    .parent = TYPE_MACHINE,
    .instance_size = sizeof(S32K3X8EVBState),
    .class_init = s32k3x8evb_class_init,
};

// 注册机器类型
static void s32k3x8evb_machine_init(void)
{
    qemu_log_mask(CPU_LOG_INT, "Registering S32K3X8EVB machine type\n");
    type_register_static(&s32k3x8evb_type);
}

type_init(s32k3x8evb_machine_init)
