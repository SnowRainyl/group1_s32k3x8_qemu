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

// 内存区域和外设基地址定义
#define S32K3_FLASH_BASE     0x00400000  // 改为实际的 Flash 起始地址
#define S32K3_SRAM0_BASE     0x20400000

#define S32K3_PERIPH_BASE    0x40000000
#define S32K3_UART_BASE      (S32K3_PERIPH_BASE + 0x00000)
#define S32K3X8EVB_FLASH_SIZE    (4 * MiB)
#define S32K3X8EVB_SRAM_SIZE     (1 * MiB)

// 系统频率定义
#define S32K3_SYSCLK_FREQ    (160 * 1000 * 1000)  // 160MHz

// ROM 设备实现
static void s32k3x8_rom_init(Object *obj)
{
   S32K3X8ROMState *s = S32K3X8_ROM(obj);

    // 只初始化 ROM 内存区域，不需要预设向量表
    memory_region_init_rom(&s->flash, obj, "s32k3x8evb.flash",
                          S32K3X8EVB_FLASH_SIZE, &error_fatal);

    // 设置只读
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

// RAM 设备实现
static void s32k3x8_ram_init(Object *obj)
{
    S32K3X8RAMState *s = S32K3X8_RAM(obj);
    
    qemu_log_mask(CPU_LOG_INT, "Initializing S32K3X8 RAM\n");
    
    memory_region_init_ram(&s->sram, obj, "s32k3x8evb.sram",
                          S32K3X8EVB_SRAM_SIZE, &error_fatal);
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

// 系统复位函数
static void s32k3x8_reset_system(S32K3X8EVBState *s)
{
    qemu_log_mask(CPU_LOG_INT, "Resetting S32K3X8EVB system\n");
    // 在这里添加任何需要的系统复位代码
}

// 板级初始化函数
static void s32k3x8evb_init(MachineState *machine)
{
    S32K3X8EVBState *s = S32K3X8EVB_MACHINE(machine);
    Error *error_local = NULL;
    DeviceState *dev;
    
    qemu_log_mask(CPU_LOG_INT, "Initializing S32K3X8EVB board\n");
    MemoryRegion *flash_alias = g_new(MemoryRegion, 1);



    
    // 1. 检查并获取系统内存
    MemoryRegion *system_memory = get_system_memory();
    if (!system_memory) {
        error_report("Failed to get system memory");
        return;
    }
    
    // 2. 初始化系统时钟
    Clock *sysclk = clock_new(OBJECT(machine), "SYSCLK");
    if (!sysclk) {
        error_report("Failed to create system clock");
        return;
    }
    clock_set_hz(sysclk, S32K3_SYSCLK_FREQ);
    
    // 3. 初始化内存设备
    s->rom = qdev_new(TYPE_S32K3X8_ROM);
    s->ram = qdev_new(TYPE_S32K3X8_RAM);
    
    if (!s->rom || !s->ram) {
        error_report("Failed to create memory devices");
        return;
    }
    
    if (!sysbus_realize_and_unref(SYS_BUS_DEVICE(s->rom), &error_local) ||
        !sysbus_realize_and_unref(SYS_BUS_DEVICE(s->ram), &error_local)) {
        error_reportf_err(error_local, "Failed to initialize memory devices: ");
        return;
    }
    
    // 4. 设置内存映射
    sysbus_mmio_map(SYS_BUS_DEVICE(s->rom), 0, S32K3_FLASH_BASE);
    sysbus_mmio_map(SYS_BUS_DEVICE(s->ram), 0, S32K3_SRAM0_BASE);

    //another name
      memory_region_init_alias(flash_alias, OBJECT(machine), "s32k3x8evb.flash_alias",
                           &S32K3X8_ROM(s->rom)->flash, 0, S32K3X8EVB_FLASH_SIZE);
    memory_region_add_subregion(system_memory, 0x00000000, flash_alias);   
   //read vector table
    if (machine->kernel_filename) {
        // 获取 Flash 内存区域的指针
        uint8_t *flash_ptr = memory_region_get_ram_ptr(&S32K3X8_ROM(s->rom)->flash);
        
        // 打开固件文件
        int fd = open(machine->kernel_filename, O_RDONLY);
        if (fd < 0) {
            error_report("Failed to open kernel file %s", machine->kernel_filename);
            return;
        }

        // 获取文件大小
        off_t size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);


        qemu_log_mask(CPU_LOG_INT, "Loading firmware, size: %ld bytes\n", size);

        // 读取固件到 Flash
        if (read(fd, flash_ptr, size) != size) {
            error_report("Failed to read kernel file");
            close(fd);
            return;
        }
        close(fd);

        // 打印加载后的向量表内容
        uint32_t *vector_table = (uint32_t *)flash_ptr;
        qemu_log_mask(CPU_LOG_INT, "Vector table after loading:\n");
        qemu_log_mask(CPU_LOG_INT, "Initial SP: 0x%08x\n", vector_table[0]);
        qemu_log_mask(CPU_LOG_INT, "Reset Vector: 0x%08x\n", vector_table[1]);
	qemu_log_mask(CPU_LOG_INT, "First 32 bytes of firmware:\n");
    for (int i = 0; i < 32; i++) {
        qemu_log_mask(CPU_LOG_INT, "%02x ", flash_ptr[i]);
        if ((i + 1) % 16 == 0) qemu_log_mask(CPU_LOG_INT, "\n");
    }
    } else {
        error_report("No firmware file specified");
        return;
    }
    // 5. 初始化ARM核心
    object_initialize_child(OBJECT(machine), "armv7m", &s->armv7m, TYPE_ARMV7M);
    
    // 配置CPU
    qdev_prop_set_string(DEVICE(&s->armv7m), "cpu-type", ARM_CPU_TYPE_NAME("cortex-m7"));
    qdev_prop_set_uint32(DEVICE(&s->armv7m), "init-svtor", S32K3_FLASH_BASE);
    qdev_prop_set_uint8(DEVICE(&s->armv7m), "num-prio-bits", 4);  // Cortex-M7 使用4位优先级
    qdev_prop_set_uint32(DEVICE(&s->armv7m), "num-irq", 240);     // S32K3X8的中断数量
    
    // 6. 设置系统连接
    object_property_set_link(OBJECT(&s->armv7m), "memory", 
                           OBJECT(system_memory), &error_abort);
    qdev_connect_clock_in(DEVICE(&s->armv7m), "cpuclk", sysclk);
    
    // 7. 实现系统总线设备
    if (!sysbus_realize(SYS_BUS_DEVICE(&s->armv7m), &error_local)) {
        error_reportf_err(error_local, "Failed to realize ARM core: ");
        return;
    }
    
    // 8. 初始化UART
    qemu_log_mask(CPU_LOG_INT, "Initializing UART\n");
    dev = qdev_new(TYPE_S32K3X8_LPUART);
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
    
    // 映射UART
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, S32K3_UART_BASE);
    
    // 9. 执行系统复位
    s32k3x8_reset_system(s);
    
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
    mc->default_ram_size = S32K3X8EVB_SRAM_SIZE;
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
