# Project name
PROJECT = firmware

# Toolchain definitions
TOOLCHAIN = arm-none-eabi-
CC = $(TOOLCHAIN)gcc
AS = $(TOOLCHAIN)gcc -x assembler-with-cpp
OBJCOPY = $(TOOLCHAIN)objcopy
SIZE = $(TOOLCHAIN)size

# Directories
FREERTOS_ROOT := ../FreeRTOS/FreeRTOS
KERNEL_DIR := $(FREERTOS_ROOT)/Source
KERNEL_PORT_DIR := $(KERNEL_DIR)/portable/GCC/ARM_CM7/r0p1

# Include paths
INCLUDES = \
    -I$(KERNEL_DIR)/include \
    -I$(KERNEL_PORT_DIR) \
    -I ../Headers \
    -I. \
    -I./config

# Processor and FPU flags
CPU = cortex-m7
FPU = fpv5-sp-d16
FLOAT_ABI = hard

# Common flags for all files
COMMON_FLAGS = -mcpu=$(CPU) -mthumb $(INCLUDES)
FPU_FLAGS = -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI)

# Compiler flags
CFLAGS = $(COMMON_FLAGS) $(FPU_FLAGS) \
    -O0 \
    -Wall -Wextra \
    -ffunction-sections -fdata-sections \
    -g3\
    -DARMCM7 \
    -MMD -MP \
    -D__FPU_PRESENT=1 \
    -D__FPU_USED=1

# Assembler flags
ASFLAGS = $(COMMON_FLAGS) $(FPU_FLAGS)

# Linker flags
LDFLAGS = $(COMMON_FLAGS) $(FPU_FLAGS) \
    -Wl,--gc-sections \
    -nostartfiles \
    -T flash_link.ld\
    -specs=nano.specs \
    -specs=nosys.specs \
    -Wl,--start-group -lc -lm -Wl,--end-group

# Source files
SRC_C = \
    main.c \
    startup_ARMCM7.c\
    system_ARMCM7.c\
    LPSPI.c

# FreeRTOS source files
FREERTOS_SRC = \
    $(KERNEL_DIR)/list.c \
    $(KERNEL_DIR)/tasks.c \
    $(KERNEL_DIR)/queue.c \
    $(KERNEL_DIR)/timers.c \
    $(KERNEL_DIR)/portable/MemMang/heap_4.c \
    $(KERNEL_PORT_DIR)/port.c

# Assembly sources
# none

# Object files
OBJS = $(SRC_C:.c=.o) $(SRC_ASM:.s=.o) $(FREERTOS_SRC:.c=.o)

# Dependency files
DEPS = $(OBJS:.o=.d)

# Output files
ELF = $(PROJECT).elf
BIN = $(PROJECT).bin
HEX = $(PROJECT).hex
MAP = $(PROJECT).map

# Default target
all: $(BIN) $(HEX) size

# Compile C sources
%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile ASM sources
%.o: %.s
	@echo "AS $<"
	@$(AS) $(ASFLAGS) -c $< -o $@

# Link
$(ELF): $(OBJS)
	@echo "LD $@"
	@$(CC) $(OBJS) $(LDFLAGS) -Wl,-Map=$(MAP) -o $@

# Create binary
$(BIN): $(ELF)
	@echo "OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@

# Create hex file
$(HEX): $(ELF)
	@echo "OBJCOPY $@"
	@$(OBJCOPY) -O ihex $< $@

# Display size
size: $(ELF)
	@echo
	@$(SIZE) -A $(ELF)

# Clean target
clean:
	@echo "Cleaning..."
	@rm -f $(OBJS)
	@rm -f $(DEPS)
	@rm -f $(ELF)
	@rm -f $(BIN)
	@rm -f $(HEX)
	@rm -f $(MAP)

# Include dependency files
-include $(DEPS)

# PHONY targets
.PHONY: all clean size

# Build info
build_info:
	@echo "Project: $(PROJECT)"
	@echo "CPU: $(CPU)"
	@echo "FPU: $(FPU)"
	@echo "FLOAT-ABI: $(FLOAT_ABI)"
