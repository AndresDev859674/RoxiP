HOST_CC = gcc
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
OBJCOPY = arm-none-eabi-objcopy

DEBUG ?= 0
BUILD_CLI ?= 0
GDB_PORT ?= 7777

COMMON_FLAGS = -mcpu=arm926ej-s -marm -ffreestanding -fno-stack-protector -Wall -Wextra -Werror -nostdlib -fno-builtin -Isrc/include

ifeq ($(DEBUG),1)
    CFLAGS = $(COMMON_FLAGS) -O0 -g -DDEBUG
else
    CFLAGS = $(COMMON_FLAGS) -O3 -flto -fomit-frame-pointer
endif

OBJS = startup.o \
       src/memory.o \
       src/drivers.o \
       src/vfs.o \
       src/graphics.o \
       src/ui.o \
       src/shell.o \
       src/kernel.o

TARGETS = roxip.bin
ifeq ($(BUILD_CLI),1)
    TARGETS += roxip-cli
endif

.PHONY: all bin elf cli run qemu debug clean clean-all

# Default target: compile kernel binary
all: $(TARGETS)

# Compile raw flat binary ONLY
bin: roxip.bin

# Compile ELF executable ONLY
elf: roxip.elf

# Compile host CLI tool ONLY
cli: roxip-cli

# Assembly Startup Rule
startup.o: startup.s
	$(AS) -mcpu=arm926ej-s startup.s -o startup.o

# C Compilation Rule for src/
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link via GCC Driver to properly pass LTO plugin and handle libgcc automatically
roxip.elf: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -Wl,-T,linker.ld -nostdlib -lgcc -o roxip.elf

roxip.bin: roxip.elf
	$(OBJCOPY) -O binary roxip.elf roxip.bin

# Emulate in QEMU
run qemu: roxip.bin
	@if [ ! -f roxip.storage ]; then \
		if [ -f ./roxip-cli ]; then \
			./roxip-cli create roxip.storage 64; \
		else \
			echo "[VFS] Creating 64MB raw storage image for QEMU..."; \
			truncate -s 64M roxip.storage 2>/dev/null || dd if=/dev/zero of=roxip.storage bs=1M count=64 status=none; \
		fi; \
	fi
	qemu-system-arm -M versatilepb -cpu arm926 -m 256M -kernel roxip.bin -drive file=roxip.storage,if=pflash,format=raw -vga none -serial stdio

# Launch QEMU frozen listening on custom GDB_PORT
debug: roxip.bin
	@if [ ! -f roxip.storage ]; then \
		truncate -s 64M roxip.storage 2>/dev/null || dd if=/dev/zero of=roxip.storage bs=1M count=64 status=none; \
	fi
	@echo "[GDB Server listening on port $(GDB_PORT)] Connect with:"
	@echo "  arm-none-eabi-gdb roxip.elf -ex 'target remote localhost:$(GDB_PORT)'"
	qemu-system-arm -M versatilepb -cpu arm926 -m 256M -kernel roxip.bin -drive file=roxip.storage,if=pflash,format=raw -vga none -serial stdio -gdb tcp::$(GDB_PORT) -S

clean:
	rm -f src/*.o *.o *.elf *.bin roxip-cli

clean-all: clean
	rm -f roxip.storage