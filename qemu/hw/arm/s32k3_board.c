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

#define INT_PFLASH_BASE         0x00400000  // 程序Flash
#define INT_PFLASH_SIZE         0x007D4000  // ~8MB (减去保留区域)

#define INT_DFLASH_BASE         0x10000000  // 数据Flash
#define INT_DFLASH_SIZE         (128 * KiB) // 128KB

#define INT_SRAM_BASE           0x20400000  // 内部SRAM
#define INT_SRAM_SIZE           0x0007FF00  // ~512KB

#define INT_SRAM_FLS_RSV_BASE   0x2047FF00  // Flash预留SRAM
#define INT_SRAM_FLS_RSV_SIZE   0x00000100  // 256B

#define INT_SRAM_NO_CACHE_BASE  0x20480000  // 不可缓存SRAM
#define INT_SRAM_NO_CACHE_SIZE  0x0003BF00  // ~240KB

#define INT_SRAM_RESULTS_BASE   0x204BBF00  // 结果SRAM
#define INT_SRAM_RESULTS_SIZE   0x00000100  // 256B

#define INT_SRAM_SHARE_BASE     0x204BC000  // 共享SRAM
#define INT_SRAM_SHARE_SIZE     (16 * KiB)  // 16KB

// 外设基地址定义
#define S32K3_PERIPH_BASE        0x40000000
#define S32K3_UART_BASE          (S32K3_PERIPH_BASE + 0x4A000) // 根据S32K3参考手册调整

// 系统频率定义
#define S32K3_SYSCLK_FREQ        (160 * 1000 * 1000)  // 160MHz

// 定义全局变量保存关键内存区域的指针
static MemoryRegion *g_pflash_region = NULL;

// 内存映射初始化函数
static void s32k3x8_initialize_memory_regions(MemoryRegion *system_memory)
{
    qemu_log_mask(CPU_LOG_INT, "\n------------------ Initialization of the memory regions ------------------\n");
    
    /* 分配所有内存区域 */
    MemoryRegion *itcm = g_new(MemoryRegion, 1);
    MemoryRegion *dtcm = g_new(MemoryRegion, 1);
    MemoryRegion *dtcm_stack = g_new(MemoryRegion, 1);
    MemoryRegion *pflash = g_new(MemoryRegion, 1);
    MemoryRegion *dflash = g_new(MemoryRegion, 1);
    MemoryRegion *sram = g_new(MemoryRegion, 1);
    MemoryRegion *sram_fls_rsv = g_new(MemoryRegion, 1);
    MemoryRegion *sram_no_cache = g_new(MemoryRegion, 1);
    MemoryRegion *sram_results = g_new(MemoryRegion, 1);
    MemoryRegion *sram_share = g_new(MemoryRegion, 1);
    
    /* ITCM 初始化 - RAM */
    qemu_log_mask(CPU_LOG_INT, "Initializing ITCM...\n");
    memory_region_init_ram(itcm, NULL, "s32k3x8.itcm", INT_ITCM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_ITCM_BASE, itcm);
    
    /* DTCM 初始化 - RAM */
    qemu_log_mask(CPU_LOG_INT, "Initializing DTCM...\n");
    memory_region_init_ram(dtcm, NULL, "s32k3x8.dtcm", INT_DTCM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_DTCM_BASE, dtcm);
    
    /* DTCM 栈区域 */
    memory_region_init_ram(dtcm_stack, NULL, "s32k3x8.dtcm_stack", INT_STACK_DTCM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_STACK_DTCM_BASE, dtcm_stack);
    
    /* Program Flash 初始化 - ROM */
    qemu_log_mask(CPU_LOG_INT, "Initializing Program Flash...\n");
    memory_region_init_rom(pflash, NULL, "s32k3x8.pflash", INT_PFLASH_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_PFLASH_BASE, pflash);
    g_pflash_region = pflash; // 保存指针供后续使用
    
    /* Data Flash 初始化 - ROM */
    qemu_log_mask(CPU_LOG_INT, "Initializing Data Flash...\n");
    memory_region_init_rom(dflash, NULL, "s32k3x8.dflash", INT_DFLASH_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_DFLASH_BASE, dflash);
    
    /* 初始化各种SRAM区域 */
    qemu_log_mask(CPU_LOG_INT, "Initializing SRAM regions...\n");
    
    memory_region_init_ram(sram, NULL, "s32k3x8.sram", INT_SRAM_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_SRAM_BASE, sram);
    
    memory_region_init_ram(sram_fls_rsv, NULL, "s32k3x8.sram_fls_rsv", INT_SRAM_FLS_RSV_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_SRAM_FLS_RSV_BASE, sram_fls_rsv);
    
    memory_region_init_ram(sram_no_cache, NULL, "s32k3x8.sram_no_cache", INT_SRAM_NO_CACHE_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_SRAM_NO_CACHE_BASE, sram_no_cache);
    
    memory_region_init_ram(sram_results, NULL, "s32k3x8.sram_results", INT_SRAM_RESULTS_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_SRAM_RESULTS_BASE, sram_results);
    
    memory_region_init_ram(sram_share, NULL, "s32k3x8.sram_share", INT_SRAM_SHARE_SIZE, &error_fatal);
    memory_region_add_subregion(system_memory, INT_SRAM_SHARE_BASE, sram_share);
    
    qemu_log_mask(CPU_LOG_INT, "Memory regions initialized successfully.\n");
}

// ROM 设备实现 - 保留但不再使用
static void s32k3x8_rom_init(Object *obj)
{
    S32K3X8ROMState *s = S32K3X8_ROM(obj);
    memory_region_init_rom(&s->flash, obj, "s32k3x8evb.flash",
                          INT_PFLASH_SIZE, &error_fatal);
    memory_region_set_readonly(&s->flash, true);
}

static void s32k3x8_rom_realize(DeviceState *dev, Error **errp)
{
    S32K3X8ROMState *s = S32K3X8_ROM(dev);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->flash);
}

static void s32k3x8_rom_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->desc = "S32K3X8 ROM";
    dc->realize = s32k3x8_rom_realize;
}

static const TypeInfo s32k3x8_rom_info = {
    .name          = TYPE_S32K3X8_ROM,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S32K3X8ROMState),
    .instance_init = s32k3x8_rom_init,
    .class_init    = s32k3x8_rom_class_init,
};

// RAM 设备实现 - 保留但不再使用
static void s32k3x8_ram_init(Object *obj)
{
    S32K3X8RAMState *s = S32K3X8_RAM(obj);
    qemu_log_mask(CPU_LOG_INT, "Initializing S32K3X8 RAM\n");
    memory_region_init_ram(&s->sram, obj, "s32k3x8evb.sram",
                          INT_SRAM_SIZE, &error_fatal);
}

static void s32k3x8_ram_realize(DeviceState *dev, Error **errp)
{
    S32K3X8RAMState *s = S32K3X8_RAM(dev);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->sram);
}

static void s32k3x8_ram_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->desc = "S32K3X8 RAM";
    dc->realize = s32k3x8_ram_realize;
}

static const TypeInfo s32k3x8_ram_info = {
    .name          = TYPE_S32K3X8_RAM,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S32K3X8RAMState),
    .instance_init = s32k3x8_ram_init,
    .class_init    = s32k3x8_ram_class_init,
};

// 板级初始化函数

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
    
    // 4. 创建PFLASH到ITCM的别名，确保中断向量表在地址0处可见
    MemoryRegion *itcm_alias = g_new(MemoryRegion, 1);
    memory_region_init_alias(itcm_alias, OBJECT(machine), "pflash-to-itcm", 
                           g_pflash_region, 0, INT_ITCM_SIZE);
    // 使用较高优先级(1)，确保这个别名覆盖已经映射的ITCM
    memory_region_add_subregion_overlap(system_memory, INT_ITCM_BASE, itcm_alias, 1);
    qemu_log_mask(CPU_LOG_INT, "Created memory alias from PFLASH to ITCM (addr 0)\n");
    
    // 5. 加载固件(如果指定)到PFLASH区域
    if (machine->kernel_filename) {
        // 使用全局保存的PFLASH区域指针获取其RAM指针
        if (!g_pflash_region) {
            error_report("Flash memory region not initialized");
            return;
        }
        
        uint8_t *flash_ptr = memory_region_get_ram_ptr(g_pflash_region);
        
        // 打开固件文件
        int fd = open(machine->kernel_filename, O_RDONLY);
        if (fd < 0) {
            error_report("Failed to open kernel file %s", machine->kernel_filename);
            return;
        }

        // 获取文件大小
        off_t size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        qemu_log_mask(CPU_LOG_INT, "Loading firmware to PFLASH, size: %ld bytes\n", size);

        // 读取固件到 Flash
        if (read(fd, flash_ptr, size) != size) {
            error_report("Failed to read kernel file");
            close(fd);
            return;
        }
        close(fd);

        // 打印加载后的向量表内容
        uint32_t *vector_table = (uint32_t *)flash_ptr;
        qemu_log_mask(CPU_LOG_INT, "Vector table after loading (in PFLASH):\n");
        qemu_log_mask(CPU_LOG_INT, "Initial SP: 0x%08x\n", vector_table[0]);
        qemu_log_mask(CPU_LOG_INT, "Reset Vector: 0x%08x\n", vector_table[1]);
        qemu_log_mask(CPU_LOG_INT, "First 32 bytes of firmware:\n");
        for (int i = 0; i < 32; i++) {
            qemu_log_mask(CPU_LOG_INT, "%02x ", flash_ptr[i]);
            if ((i + 1) % 16 == 0) qemu_log_mask(CPU_LOG_INT, "\n");
        }
        
        // 验证ITCM别名是否正常工作
        uint8_t *itcm_ram = memory_region_get_ram_ptr(itcm_alias);
        uint32_t initial_sp_itcm = *(uint32_t *)itcm_ram;
        qemu_log_mask(CPU_LOG_INT, "Initial SP at ITCM alias (addr 0): 0x%08x (should match PFLASH)\n", 
                     initial_sp_itcm);
    } else {
        error_report("No firmware file specified");
        return;
    }
    
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
    mc->default_ram_size = INT_SRAM_SIZE;
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
    type_register_static(&s32k3x8_rom_info);
    type_register_static(&s32k3x8_ram_info);
    type_register_static(&s32k3x8evb_type);
}

type_init(s32k3x8evb_machine_init)
