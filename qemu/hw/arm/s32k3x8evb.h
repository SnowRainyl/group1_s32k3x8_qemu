// hw/arm/s32k3x8evb.h
#ifndef HW_ARM_S32K3X8EVB_H
#define HW_ARM_S32K3X8EVB_H

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qom/object.h"
#include "hw/boards.h"
#include "hw/arm/armv7m.h"
#include "hw/sysbus.h"

// 定义内存大小
#define S32K3X8EVB_FLASH_SIZE (4 * MiB)  
#define S32K3X8EVB_SRAM_SIZE  (1 * MiB)
//define the size of memory--mapping
//#define S32K3_SRAM0_BASE     0x40264000  //PRAMC_0 
//#define S32K3_FLASH_BASE     0x00400000  // FLASH 

#define S32K3_FLASH_BASE 0x00000000  // 修改Flash基地址
#define S32K3_SRAM0_BASE 0x20000000

// 定义设备类型标识符
#define TYPE_S32K3X8EVB        TYPE_S32K3X8EVB_MACHINE
#define TYPE_S32K3X8EVB_MACHINE "s32k3x8evb-machine"
// ROM 设备定义
#define TYPE_S32K3X8_ROM "s32k3x8.rom"
#define VECTOR_TABLE_BASE S32K3_FLASH_BASE


typedef struct S32K3X8ROMState {
    SysBusDevice parent_obj;
    MemoryRegion flash;
} S32K3X8ROMState;

// RAM 设备定义
#define TYPE_S32K3X8_RAM "s32k3x8.ram"

typedef struct S32K3X8RAMState {
    SysBusDevice parent_obj;
    MemoryRegion sram;
} S32K3X8RAMState;


// 板级状态结构体
typedef struct S32K3X8EVBState {
    MachineState parent_obj;
    DeviceState *rom;
    DeviceState *ram;
    ARMv7MState armv7m;
    DeviceState *uart;
    DeviceState *spi;
} S32K3X8EVBState;

// 声明状态检查宏
DECLARE_INSTANCE_CHECKER(S32K3X8EVBState, S32K3X8EVB_MACHINE, 
                        TYPE_S32K3X8EVB_MACHINE)
DECLARE_INSTANCE_CHECKER(S32K3X8ROMState, S32K3X8_ROM, TYPE_S32K3X8_ROM)
DECLARE_INSTANCE_CHECKER(S32K3X8RAMState, S32K3X8_RAM, TYPE_S32K3X8_RAM)

#endif
