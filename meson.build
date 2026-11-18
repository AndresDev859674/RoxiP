# meson.build - RoxiP build system (Meson + Ninja)
# Replaces the legacy Makefile.
#
# Usage:
#   meson setup builddir --cross-file cross-arm-none-eabi.txt -Dboard=versatilepb
#   meson compile -C builddir
#
# Boards: versatilepb (default), rockchip, amlogic

project('roxip', 'c',
  version: '0.1.0',
  license: 'GPL',
  meson_version: '>=0.60.0',
  default_options: [
    'buildtype=release',
    'warning_level=3',
  ])

# ---------------------------------------------------------------------------
# Options (declared in meson_options.txt)
# ---------------------------------------------------------------------------
board = get_option('board')
debug = get_option('roxip-debug')
build_cli = get_option('build-cli')
gdb_port = get_option('gdb-port')

# ---------------------------------------------------------------------------
# Architecture selection based on board
# ---------------------------------------------------------------------------
if board == 'rockchip'
  arch_cpu = 'cortex-a7'
elif board == 'amlogic'
  arch_cpu = 'cortex-a53'
elif board == 'riscv-virt'
  arch_cpu = 'rv64gc'
else
  arch_cpu = 'arm926ej-s'
endif

is_riscv = board == 'riscv-virt'

message('RoxiP building for board: @0@ (cpu: @1@)'.format(board, arch_cpu))

# ---------------------------------------------------------------------------
# Compiler flags (ARM bare-metal or RISC-V bare-metal)
# ---------------------------------------------------------------------------
source_root = meson.project_source_root()

if is_riscv
  c_args = [
    '-march=' + arch_cpu,
    '-mabi=lp64',
    '-mcmodel=medany',
    '-ffreestanding',
    '-fno-stack-protector',
    '-Wall', '-Wextra', '-Werror',
    '-nostdlib',
    '-fno-builtin',
    '-I' + source_root + '/src/include',
    '-I' + source_root + '/firmware/' + board,
    '-D__riscv__',
  ]
else
  c_args = [
    '-mcpu=' + arch_cpu,
    '-marm',
    '-ffreestanding',
    '-fno-stack-protector',
    '-Wall', '-Wextra', '-Werror',
    '-nostdlib',
    '-fno-builtin',
    '-I' + source_root + '/src/include',
    '-I' + source_root + '/firmware/' + board,
  ]
endif

if debug
  c_args += ['-O0', '-g', '-DDEBUG']
  message('Debug build enabled')
else
  c_args += ['-O3', '-flto', '-fomit-frame-pointer']
endif

# ---------------------------------------------------------------------------
# Compilers / tools
# ---------------------------------------------------------------------------
if is_riscv
  armgcc = find_program('riscv64-elf-gcc')
  armas = find_program('riscv64-elf-as')
  objcopy = find_program('riscv64-elf-objcopy')
  startup = files('startup_riscv.S')
else
  armgcc = find_program('arm-none-eabi-gcc')
  armas = find_program('arm-none-eabi-as')
  objcopy = find_program('arm-none-eabi-objcopy')
  startup = files('startup.s')
endif

# ---------------------------------------------------------------------------
# Compile objects
# ---------------------------------------------------------------------------
if is_riscv
  startup_obj = custom_target('startup-obj',
    input: startup,
    output: 'startup.o',
    command: [armas, '-march=' + arch_cpu, '-mabi=lp64', '@INPUT@', '-o', '@OUTPUT@'],
  )
else
  startup_obj = custom_target('startup-obj',
    input: startup,
    output: 'startup.o',
    command: [armas, '-mcpu=' + arch_cpu, '@INPUT@', '-o', '@OUTPUT@'],
  )
endif

# Firmware objects (board-specific)
firmware_board = files('firmware/@0@/board.c'.format(board))
firmware_system = files('firmware/@0@/system.c'.format(board))

firmware_objs = [
  custom_target('firmware-board-obj',
    input: firmware_board,
    output: 'board.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('firmware-system-obj',
    input: firmware_system,
    output: 'system.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
]

# Driver objects
driver_objs = [
  custom_target('driver-uart-obj',
    input: files('src/drivers/uart.c'),
    output: 'uart.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('driver-display-obj',
    input: files('src/drivers/display.c'),
    output: 'display.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('driver-hdmi-obj',
    input: files('src/drivers/hdmi.c'),
    output: 'hdmi.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('driver-input-obj',
    input: files('src/drivers/input.c'),
    output: 'input.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('driver-storage-audio-obj',
    input: files('src/drivers/storage_audio.c'),
    output: 'storage_audio.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('driver-fat32-obj',
    input: files('src/drivers/fat32.c'),
    output: 'fat32.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
]

# UI objects
ui_objs = [
  custom_target('ui-core-obj',
    input: files('src/ui/ui_core.c'),
    output: 'ui_core.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('ui-cli-obj',
    input: files('src/ui/ui_cli.c'),
    output: 'ui_cli.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('ui-desktop-obj',
    input: files('src/ui/ui_desktop.c'),
    output: 'ui_desktop.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('ui-apps-obj',
    input: files('src/ui/ui_apps.c'),
    output: 'ui_apps.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('ui-demos-obj',
    input: files('src/ui/ui_demos.c'),
    output: 'ui_demos.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
]

# Kernel objects
kernel_objs = [
  custom_target('kernel-memory-obj',
    input: files('src/memory.c'),
    output: 'memory.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('kernel-yafs-obj',
    input: files('src/yafs.c'),
    output: 'yafs.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('kernel-graphics-obj',
    input: files('src/graphics.c'),
    output: 'graphics.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('kernel-shell-obj',
    input: files('src/shell.c'),
    output: 'shell.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
  custom_target('kernel-kernel-obj',
    input: files('src/kernel.c'),
    output: 'kernel.o',
    command: [armgcc] + c_args + ['-c', '@INPUT@', '-o', '@OUTPUT@'],
  ),
]

all_objs = [startup_obj] + firmware_objs + driver_objs + ui_objs + kernel_objs

# ---------------------------------------------------------------------------
# Link: roxip.elf
# ---------------------------------------------------------------------------
roxip_elf = custom_target('roxip-elf',
  input: all_objs,
  output: 'roxip.elf',
  command: [armgcc] + c_args + ['-o', '@OUTPUT@', '@INPUT@', '-Wl,-T,@SOURCE_ROOT@/' + (is_riscv ? 'linker_riscv.ld' : 'linker.ld'), '-nostdlib', '-lgcc'],
)

# ---------------------------------------------------------------------------
# objcopy: roxip.elf -> roxip.bin
# ---------------------------------------------------------------------------
roxip_bin = custom_target('roxip-bin',
  input: roxip_elf,
  output: 'roxip.bin',
  command: [objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@'],
)

# ---------------------------------------------------------------------------
# Host CLI tool (native build, not cross-compiled)
# ---------------------------------------------------------------------------
if build_cli
  hostcc = find_program('gcc')

  roxip_cli = custom_target('roxip-cli',
    input: files('src/cli/roxip-cli.c'),
    output: 'roxip-cli',
    command: [hostcc, '-O2', '-Wall', '-Wextra', '@INPUT@', '-o', '@OUTPUT@'],
  )
endif

# ---------------------------------------------------------------------------
# QEMU run targets (meson equivalents of `make run`)
# ---------------------------------------------------------------------------
qemu_arm = find_program('qemu-system-arm', required: false)
qemu_riscv = find_program('qemu-system-riscv64', required: false)

build_dir = meson.current_build_dir()

if qemu_arm.found() or qemu_riscv.found()
  # Storage image (64 MB raw)
  roxip_storage = custom_target('roxip-storage',
    output: 'roxip.storage',
    command: [
      'sh', '-c',
      'if [ ! -f @OUTPUT@ ]; then truncate -s 64M @OUTPUT@ 2>/dev/null || dd if=/dev/zero of=@OUTPUT@ bs=1M count=64 status=none; fi',
    ],
  )

  # Combined image: roxip.bin embedded at offset 0, followed by roxip.storage
  # QEMU loads the kernel via -kernel and mounts roxip.img as the storage drive.
  roxip_img = custom_target('roxip-img',
    input: [roxip_bin, roxip_storage],
    output: 'roxip.img',
    command: [
      'sh', '-c',
      'cp @INPUT1@ @OUTPUT@ && dd if=@INPUT0@ of=@OUTPUT@ conv=notrunc 2>/dev/null',
    ],
  )

  if is_riscv
    qemu_cmd = [
      qemu_riscv, '-M', 'virt', '-m', '256M',
      '-kernel', build_dir + '/roxip.bin',
      '-drive', 'file=' + build_dir + '/roxip.img,if=none,format=raw,id=hd0',
      '-device', 'virtio-blk-device,drive=hd0',
      '-vga', 'none', '-serial', 'stdio', '-audio', 'driver=none',
      '-nographic',
    ]
    qemu_msg = 'qemu-system-riscv64 -M virt'
  else
    qemu_cmd = [
      qemu_arm, '-M', 'versatilepb', '-cpu', 'arm926', '-m', '256M',
      '-kernel', build_dir + '/roxip.bin',
      '-drive', 'file=' + build_dir + '/roxip.img,if=pflash,format=raw',
      '-vga', 'none', '-serial', 'stdio', '-audio', 'driver=none',
    ]
    qemu_msg = 'qemu-system-arm -M versatilepb'
  endif

  # `meson compile -C builddir roxip-run`  → builds roxip.bin + img, launches QEMU
  # Shows boot logs on the serial console (terminal output).
  roxip_run = custom_target('roxip-run',
    input: [roxip_bin, roxip_img],
    output: '.roxip-run-stamp',
    command: qemu_cmd,
  )

  # `meson compile -C builddir roxip-img` → builds the combined roxip.img file
  message('')
  message('To run in QEMU (meson equivalent of `make run`):')
  message('  meson compile -C builddir roxip-run   (' + qemu_msg + ')')
  message('  (shows serial boot logs, press Ctrl+C to stop QEMU)')
  message('')
  message('To package a monolithic system image:')
  message('  meson compile -C builddir roxip-img   (produces builddir/roxip.img)')
  message('')
else
  warning('No QEMU emulator found; install qemu-system-arm or qemu-system-riscv64 to test in QEMU')
endif
