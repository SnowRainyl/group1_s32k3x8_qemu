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
#include "hw/char/s32k3x8_spi.h"
#include "hw/arm/boot.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-clock.h"
#include "hw/clock.h"
#include <stdio.h>

// ROM 设备实现
static void s32k3x8_rom_init(Object *obj)
{
    S32K3X8ROMState *s = S32K3X8_ROM(obj);
    memory_region_init_rom(&s->flash, obj, "s32k3x8evb.flash",
                          S32K3X8EVB_FLASH_SIZE, &error_fatal);
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

static void s32k3x8evb_init(MachineState *machine)
{
    S32K3X8EVBState *s = S32K3X8EVB_MACHINE(machine);
    DeviceState *dev;
    
    printf("initial clock begin!\n");
    Clock *sysclk = clock_new(OBJECT(machine), "SYSCLK");
    printf("initial clock begin 1!\n");
    clock_set_hz(sysclk, 8000000);
    printf("initial clock begin 1 finish!\n");
    
    printf("initial rom begin!\n");
    // 创建 ROM 和 RAM 设备
    s->rom = qdev_new(TYPE_S32K3X8_ROM);
    s->ram = qdev_new(TYPE_S32K3X8_RAM);
    
    // 实现设备
    sysbus_realize_and_unref(SYS_BUS_DEVICE(s->rom), &error_fatal);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(s->ram), &error_fatal);
    
    // 映射内存区域
    printf("mapping begin\n");
    sysbus_mmio_map(SYS_BUS_DEVICE(s->rom), 0, S32K3_FLASH_BASE);
    printf("mapping begin 1\n");
    sysbus_mmio_map(SYS_BUS_DEVICE(s->ram), 0, S32K3_SRAM0_BASE);

    printf("initial rom/ram finish!\n");

    printf("mapping begin2\n");
    printf("Flash base: 0x%08x, size: 0x%08lx\n", S32K3_FLASH_BASE, S32K3X8EVB_FLASH_SIZE);
    printf("SRAM base: 0x%08x, size: 0x%08lx\n", S32K3_SRAM0_BASE, S32K3X8EVB_SRAM_SIZE);

    printf("1\n");
    object_initialize_child(OBJECT(machine), "armv7m", &s->armv7m, TYPE_ARMV7M);
    printf("2\n");
    qdev_prop_set_string(DEVICE(&s->armv7m), "cpu-type", ARM_CPU_TYPE_NAME("cortex-m7"));
    printf("3\n");
    object_property_set_link(OBJECT(&s->armv7m), "memory", OBJECT(get_system_memory()), &error_abort);
    printf("4\n");
    qdev_connect_clock_in(DEVICE(&s->armv7m), "cpuclk", sysclk);
    printf("5\n");
    sysbus_realize(SYS_BUS_DEVICE(&s->armv7m), &error_abort);
    printf("6\n");

    printf("initial uart board!\n\n");
    dev = qdev_new(TYPE_S32K3X8_LPUART);
    if (!dev) {
        error_report("Failed to create UART device");
        return;
    }
    
    qdev_prop_set_chr(dev, "chardev", serial_hd(0));
    s->uart = dev;
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);
    printf("uart mapping \n");
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0x40000000);
    printf("uart mapping finished!\n");
    
    
    printf("initial lpspi board!\n\n");
    dev = qdev_new(TYPE_S32K3X8_SPI);
    if (!dev) {
        error_report("Failed to create LPSPI device");
        return;
    }
    qdev_connect_clock_in(dev, "spi_clk", sysclk);  // Connect the system clock
    qdev_prop_set_chr(dev, "chardev", serial_hd(1));
    s->spi = dev;
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);

    // Map LPSPI to the corresponding base address
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0x40358000);
    printf("LPSPI mapping finished!\n");
}

static void s32k3x8evb_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    mc->desc = "NXP S32K3X8EVB Development Board (Cortex-M7)";
    mc->init = s32k3x8evb_init;
    mc->default_cpus = 1;
}

static const TypeInfo s32k3x8evb_type = {
    .name = MACHINE_TYPE_NAME("s32k3x8evb"),
    .parent = TYPE_MACHINE,
    .instance_size = sizeof(S32K3X8EVBState),
    .class_init = s32k3x8evb_class_init,
};

static void s32k3x8evb_machine_init(void)
{
    printf("Registering S32K3X8EVB machine type: %s\n", TYPE_S32K3X8EVB_MACHINE);
    type_register_static(&s32k3x8_rom_info);
    type_register_static(&s32k3x8_ram_info);
    type_register_static(&s32k3x8evb_type);
    printf("Registration complete for type: %s\n", s32k3x8evb_type.name);
}

type_init(s32k3x8evb_machine_init)
