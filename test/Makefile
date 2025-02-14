# Toolchain definitions
TOOLCHAIN = arm-none-eabi-
CC = $(TOOLCHAIN)gcc
OBJCOPY = $(TOOLCHAIN)objcopy

# Processor specific flags
CPU = cortex-m7
CFLAGS = -mcpu=$(CPU) -mthumb
ASFLAGS = $(CFLAGS) -x assembler-with-cpp

# Linker configuration
LDFLAGS = $(CFLAGS) -nostartfiles -T linker_flash_s32k358.ld

# Source files
STARTUP_ASM = startup_cm7.s
STARTUP_C = startup.c
MAIN = main.c

# Object files
OBJS = startup_cm7.o startup.o main.o

# Output files
ELF = firmware.elf
BIN = firmware.bin

# Default target
all: $(BIN)

# Compile assembly files
%.o: %.s
	$(CC) $(ASFLAGS) -c $< -o $@

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(ELF): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

# Generate binary
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# Clean target
clean:
	rm -f $(OBJS) $(ELF) $(BIN)

# PHONY targets
.PHONY: all clean

# Build rules for specific source files
startup_cm7.o: $(STARTUP_ASM)
	$(CC) $(ASFLAGS) -c $< -o $@

startup.o: $(STARTUP_C)
	$(CC) $(CFLAGS) -c $< -o $@

main.o: $(MAIN)
	$(CC) $(CFLAGS) -c $< -o $@
