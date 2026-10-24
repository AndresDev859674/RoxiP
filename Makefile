# Makefile for RoxiP OS & Host Storage CLI
HOST_CC = gcc
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

CFLAGS = -mcpu=arm926ej-s -marm -ffreestanding -fno-stack-protector -O2 -Wall -Wextra -nostdlib -fno-builtin
LDFLAGS = -T linker.ld -nostdlib

all: roxip.bin roxip-cli

# Host CLI Binary Tool Rule
roxip-cli: cli/roxip-cli.c
	$(HOST_CC) -O2 -Wall -Wextra cli/roxip-cli.c -o roxip-cli

# Kernel ARM Targets
startup.o: startup.s
	$(AS) -mcpu=arm926ej-s startup.s -o startup.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

roxip.elf: startup.o kernel.o
	$(LD) $(LDFLAGS) startup.o kernel.o /usr/lib/gcc/arm-none-eabi/16.2.0/thumb/v7/nofp/libgcc.a -o roxip.elf

roxip.bin: roxip.elf
	$(OBJCOPY) -O binary roxip.elf roxip.bin

run: roxip.bin roxip-cli
	@if [ ! -f roxip.storage ]; then ./roxip-cli create roxip.storage 64; fi
	qemu-system-arm -M versatilepb -cpu arm926 -m 256M -kernel roxip.bin -drive file=roxip.storage,if=pflash,format=raw -vga none -serial stdio

clean:
	rm -f *.o *.elf *.bin roxip-cli

clean-all: clean
	rm -f roxip.storage
