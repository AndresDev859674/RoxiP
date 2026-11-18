HOST_CC = gcc
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
OBJCOPY = arm-none-eabi-objcopy

BOARD ?= versatilepb
DEBUG ?= 0
BUILD_CLI ?= 0
GDB_PORT ?= 7777
NETWORK_ENABLED ?= 0

# Legacy board-selection flags (kept for backward compatibility)
ROCKCHIP ?= 0
AMLOGIC ?= 0
RISCVVIRT ?= 0

# Force board when corresponding legacy flag is passed
ifeq ($(ROCKCHIP),1)
    BOARD = rockchip
endif
ifeq ($(AMLOGIC),1)
    BOARD = amlogic
endif
ifeq ($(RISCVVIRT),1)
    BOARD = riscv-virt
endif

# Select toolchain and CPU target architecture based on active board
ifeq ($(BOARD),riscv-virt)
    CC = riscv64-elf-gcc
    AS = riscv64-elf-as
    OBJCOPY = riscv64-elf-objcopy
    ARCH_CPU = rv64gc
    ABI_FLAGS = -march=rv64gc -mabi=lp64 -mcmodel=medany
    AS_FLAGS = -march=rv64gc -mabi=lp64
    CFLAGS_BASE = -ffreestanding -fno-stack-protector -Wall -Wextra -Werror -nostdlib -fno-builtin -Isrc/include -Isrc/net -Isrc/drivers -Ifirmware/$(BOARD) -D__riscv__
    STARTUP_SRC = startup_riscv.S
else
    ARCH_CPU = arm926ej-s
    ifeq ($(BOARD),rockchip)
        ARCH_CPU = cortex-a7
    else ifeq ($(BOARD),amlogic)
        ARCH_CPU = cortex-a53
    endif
    ABI_FLAGS = -mcpu=$(ARCH_CPU) -marm
    AS_FLAGS = -mcpu=$(ARCH_CPU)
    CFLAGS_BASE = -ffreestanding -fno-stack-protector -Wall -Wextra -Werror -nostdlib -fno-builtin -Isrc/include -Isrc/net -Isrc/drivers -Ifirmware/$(BOARD)
    STARTUP_SRC = startup.s
endif

COMMON_FLAGS = $(ABI_FLAGS) $(CFLAGS_BASE)

ifeq ($(NETWORK_ENABLED),1)
    COMMON_FLAGS += -DNETWORK_ENABLED -DNET_ARP -DNET_IP -DNET_ICMP -DNET_UDP
endif

ifeq ($(BOARD),riscv-virt)
    # RISC-V LTO has issues; disable LTO
    CFLAGS = $(COMMON_FLAGS) -O2 -g -fno-lto
else
ifeq ($(DEBUG),1)
    CFLAGS = $(COMMON_FLAGS) -O0 -g -DDEBUG
else
    CFLAGS = $(COMMON_FLAGS) -O3 -flto -fomit-frame-pointer
endif
endif

FIRMWARE_SRCS = $(wildcard firmware/$(BOARD)/*.c)
FIRMWARE_OBJS = $(FIRMWARE_SRCS:.c=.o)

DRIVER_OBJS = src/drivers/uart.o \
              src/drivers/display.o \
              src/drivers/hdmi.o \
              src/drivers/input.o \
              src/drivers/storage_audio.o \
              src/drivers/fat32.o

ifeq ($(NETWORK_ENABLED),1)
    DRIVER_OBJS += src/drivers/ne2000.o
    NET_OBJS = src/net/net.o \
               src/net/arp.o \
               src/net/ip.o \
               src/net/icmp.o \
               src/net/udp.o
else
    NET_OBJS =
endif

UI_OBJS = src/ui/ui_core.o \
          src/ui/ui_cli.o \
          src/ui/ui_desktop.o \
          src/ui/ui_apps.o \
          src/ui/ui_demos.o

OBJS = startup.o \
       $(FIRMWARE_OBJS) \
       $(DRIVER_OBJS) \
       $(NET_OBJS) \
       src/memory.o \
       src/yafs.o \
       src/graphics.o \
       src/shell.o \
       src/kernel.o \
       $(UI_OBJS)

TARGETS = roxip.bin
ifeq ($(BUILD_CLI),1)
    TARGETS += roxip-cli
endif

.PHONY: all bin elf cli img img-run run qemu debug rockchip amlogic riscv-virt \
        run-rockchip run-amlogic run-riscv-virt clean clean-all

# Default target
all: $(TARGETS)

# Target shortcut to build for Rockchip platform
rockchip:
	@$(MAKE) BOARD=rockchip all

# Target shortcut to build for Amlogic Meson platform
amlogic:
	@$(MAKE) BOARD=amlogic all

# Target shortcut to build for RISC-V virt platform
riscv-virt:
	@$(MAKE) BOARD=riscv-virt all

# Convenience: build and run for a specific board
run-rockchip:
	@$(MAKE) BOARD=rockchip run qemu

run-amlogic:
	@$(MAKE) BOARD=amlogic run qemu

run-riscv-virt:
	@$(MAKE) BOARD=riscv-virt run qemu

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
startup.o: $(STARTUP_SRC)
ifeq ($(BOARD),riscv-virt)
	$(AS) -march=$(ARCH_CPU) -mabi=lp64 $(STARTUP_SRC) -o startup.o
else
	$(AS) $(AS_FLAGS) $(STARTUP_SRC) -o startup.o
endif

# C Compilation Rule for firmware/
firmware/$(BOARD)/%.o: firmware/$(BOARD)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# C Compilation Rule for src/drivers/
src/drivers/%.o: src/drivers/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# C Compilation Rule for src/
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# C Compilation Rule for src/ui/
src/ui/%.o: src/ui/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link via GCC Driver
roxip.elf: $(OBJS)
ifeq ($(BOARD),riscv-virt)
	$(CC) $(CFLAGS) $(OBJS) -Wl,-T,linker_riscv.ld -nostdlib -lgcc -o roxip.elf
else
	$(CC) $(CFLAGS) $(OBJS) -Wl,-T,linker.ld -nostdlib -lgcc -o roxip.elf
endif

roxip.bin: roxip.elf
	$(OBJCOPY) -O binary roxip.elf roxip.bin

# Target for packaging Monolithic System Image
# roxip.img contains roxip.bin embedded at offset 0, followed by roxip.storage.
# QEMU loads the kernel via -kernel and mounts roxip.img as the storage drive.
img: roxip.bin
	@echo "[IMG] Ensuring persistent storage file exists..."
	@if [ ! -f roxip.storage ]; then \
		if [ -f ./roxip-cli ]; then \
			./roxip-cli create roxip.storage 64; \
		else \
			echo "[YAFS] Creating 64MB raw storage image for QEMU..."; \
			truncate -s 64M roxip.storage 2>/dev/null || dd if=/dev/zero of=roxip.storage bs=1M count=64 status=none; \
		fi; \
	else \
		echo "[IMG] Storage file roxip.storage already exists — skipping creation."; \
	fi
	@echo "[IMG] Generating roxip.img (kernel embedded at offset 0 + storage)..."
	@cp roxip.storage roxip.img
	@dd if=roxip.bin of=roxip.img conv=notrunc 2>/dev/null
	@echo "[IMG] System image roxip.img ready (kernel @ 0x00000000, storage @ 0x00001000)."

# Run monolithic .img directly in QEMU
img-run: img
	@if [ "$(BOARD)" = "riscv-virt" ]; then \
		echo "[QEMU] Booting RoxiP Kernel (RISC-V virt) from roxip.img..."; \
		qemu-system-riscv64 -M virt -m 256M -kernel roxip.bin -drive file=roxip.img,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0 -vga none -serial stdio -audio driver=none -nographic; \
	else \
		echo "[QEMU] Booting RoxiP Kernel from roxip.img..."; \
		qemu-system-arm -M versatilepb -cpu arm926 -m 256M -kernel roxip.bin -drive file=roxip.img,if=pflash,format=raw -vga none -serial stdio -audio driver=none; \
	fi

# Emulate in QEMU (Standard mode)
run qemu: roxip.bin
	@if [ ! -f roxip.storage ]; then \
		if [ -f ./roxip-cli ]; then \
			./roxip-cli create roxip.storage 64; \
		else \
			echo "[YAFS] Creating 64MB raw storage image for QEMU..."; \
			truncate -s 64M roxip.storage 2>/dev/null || dd if=/dev/zero of=roxip.storage bs=1M count=64 status=none; \
		fi; \
	else \
		echo "[YAFS] Storage file roxip.storage already exists — skipping creation."; \
	fi
	@if [ "$(BOARD)" = "riscv-virt" ]; then \
		echo "[QEMU] Booting RoxiP (RISC-V virt)..."; \
		qemu-system-riscv64 -M virt -m 256M -kernel roxip.bin -drive file=roxip.storage,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0 -vga none -serial stdio -audio driver=none -nographic; \
	else \
		qemu-system-arm -M versatilepb -cpu arm926 -m 256M -kernel roxip.bin -drive file=roxip.storage,if=pflash,format=raw -vga none -serial stdio -audio driver=none; \
	fi
	
# Launch QEMU frozen listening on custom GDB_PORT
debug: roxip.bin
	@if [ ! -f roxip.storage ]; then \
		truncate -s 64M roxip.storage 2>/dev/null || dd if=/dev/zero of=roxip.storage bs=1M count=64 status=none; \
	else \
		echo "[DEBUG] Storage file roxip.storage already exists — skipping creation."; \
	fi
	@if [ "$(BOARD)" = "riscv-virt" ]; then \
		@echo "[GDB Server listening on port $(GDB_PORT)] Connect with:"; \
		@echo "  riscv64-unknown-elf-gdb roxip.elf -ex 'target remote localhost:$(GDB_PORT)'"; \
		qemu-system-riscv64 -M virt -m 256M -kernel roxip.bin -drive file=roxip.storage,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0 -vga none -serial stdio -audio driver=none -gdb tcp::$(GDB_PORT) -S -nographic; \
	else \
		@echo "[GDB Server listening on port $(GDB_PORT)] Connect with:"; \
		@echo "  arm-none-eabi-gdb roxip.elf -ex 'target remote localhost:$(GDB_PORT)'"; \
		qemu-system-arm -M versatilepb -cpu arm926 -m 256M -kernel roxip.bin -drive file=roxip.storage,if=pflash,format=raw -vga none -serial stdio -gdb tcp::$(GDB_PORT) -S; \
	fi

clean:
	rm -f firmware/*/*.o src/drivers/*.o src/*.o src/net/*.o src/ui/*.o *.o *.elf *.bin roxip-cli roxip.img roxip.storage startup_riscv.o

clean-all: clean
	rm -f roxip.storage