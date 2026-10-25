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

UI_OBJS = src/ui/ui_core.o \
          src/ui/ui_cli.o \
          src/ui/ui_desktop.o \
          src/ui/ui_apps.o \
          src/ui/ui_demos.o

OBJS = startup.o \
       src/memory.o \
       src/drivers.o \
       src/vfs.o \
       src/graphics.o \
       src/shell.o \
       src/kernel.o \
       $(UI_OBJS)

TARGETS = roxip.bin
ifeq ($(BUILD_CLI),1)
    TARGETS += roxip-cli
endif

.PHONY: all bin elf cli img img-run run qemu debug clean clean-all

# Default target
all: $(TARGETS)

# Target for raw binary only
bin: roxip.bin

# Target for ELF output only
elf: roxip.elf

# Target for host CLI tool only
cli: roxip-cli

# Host CLI Tool Build Rule
roxip-cli: src/cli/roxip-cli.c
	$(HOST_CC) -O2 -Wall -Wextra src/cli/roxip-cli.c -o roxip-cli

# Assembly Startup Rule
startup.o: startup.s
	$(AS) -mcpu=arm926ej-s startup.s -o startup.o

# C Compilation Rule for src/
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# C Compilation Rule for src/ui/
src/ui/%.o: src/ui/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link via GCC Driver
roxip.elf: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -Wl,-T,linker.ld -nostdlib -lgcc -o roxip.elf

roxip.bin: roxip.elf
	$(OBJCOPY) -O binary roxip.elf roxip.bin

# Target for packaging Monolithic System Image
img: roxip.bin
	@echo "[IMG] Ensuring persistent storage file exists..."
	@if [ ! -f roxip.storage ]; then \
		if [ -f ./roxip-cli ]; then \
			./roxip-cli create roxip.storage 64; \
		else \
			truncate -s 64M roxip.storage 2>/dev/null || dd if=/dev/zero of=roxip.storage bs=1M count=64 status=none; \
		fi; \
	fi
	@echo "[IMG] Generating roxip.img as raw storage image..."
	@cp roxip.storage roxip.img
	@echo "[IMG] System image roxip.img ready."

# Run monolithic .img directly in QEMU
img-run: img
	@echo "[QEMU] Booting RoxiP Kernel with roxip.img storage..."
	qemu-system-arm -M versatilepb -cpu arm926 -m 256M -kernel roxip.bin -drive file=roxip.img,if=pflash,format=raw -vga none -serial stdio

# Emulate in QEMU (Standard mode)
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
	rm -f src/*.o src/ui/*.o *.o *.elf *.bin roxip-cli roxip.img

clean-all: clean
	rm -f roxip.storage