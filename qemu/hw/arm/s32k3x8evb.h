// hw/arm/s32k3x8evb.h
#ifndef HW_ARM_S32K3X8EVB_H
#define HW_ARM_S32K3X8EVB_H

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qom/object.h"
#include "hw/boards.h"
#include "hw/arm/armv7m.h"
#include "hw/sysbus.h"

// Define memory sizes
#define S32K3X8EVB_FLASH_SIZE (4 * MiB)
#define S32K3X8EVB_SRAM_SIZE  (1 * MiB)

// Define memory-mapped base addresses
#define S32K3_FLASH_BASE 0x00400000  // Flash base address
#define S32K3_SRAM0_BASE 0x20400000  // SRAM base address

// Define device type identifiers
#define TYPE_S32K3X8EVB         TYPE_S32K3X8EVB_MACHINE
#define TYPE_S32K3X8EVB_MACHINE "s32k3x8evb-machine"

// ROM device type definition
#define VECTOR_TABLE_BASE S32K3_FLASH_BASE


// Number of LPSPI instances
#define S32K3X8_LPSPI_COUNT 6

// Board state structure - includes support for LPSPI
typedef struct S32K3X8EVBState {
    MachineState parent_obj;
    ARMv7MState armv7m;
    DeviceState *uart;
    /* Added: array of LPSPI device instances (LPSPI0 to LPSPI5) */
    DeviceState *lpspi[S32K3X8_LPSPI_COUNT];
} S32K3X8EVBState;

// Declare instance checker macros
DECLARE_INSTANCE_CHECKER(S32K3X8EVBState, S32K3X8EVB_MACHINE,
                         TYPE_S32K3X8EVB_MACHINE)


#endif
