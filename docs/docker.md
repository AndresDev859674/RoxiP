# i need docker! but RoxiP has it!

Docker lets you build and test RoxiP without installing any cross-compilers,
Meson, Ninja, or QEMU on your host machine. Everything you need is inside the
container.

## Files

| File | Purpose |
|------|---------|
| `Dockerfile` | Container image with all build dependencies |
| `.dockerignore` | Files excluded from the Docker build context |
| `docker-compose.yml` | Pre-configured services for each board |

## Prerequisites

- [Docker](https://docs.docker.com/get-docker/) installed on your host
- For QEMU emulation inside Docker: `--privileged` or KVM device access
  (`/dev/kvm`)

## Quick Start

### 1. Build the Docker image

```bash
docker build -t roxip .
```

### 2. Build RoxiP for a board

**VersatilePB (default, easiest to test):**

```bash
docker run --rm -it -v $(pwd):/home/roxip/roxip roxip make img
```

**Rockchip:**

```bash
docker run --rm -it -v $(pwd):/home/roxip/roxip roxip make rockchip img
```

**Amlogic Meson:**

```bash
docker run --rm -it -v $(pwd):/home/roxip/roxip roxip make amlogic img
```

**RISC-V virt:**

```bash
docker run --rm -it -v $(pwd):/home/roxip/roxip roxip make riscv-virt img
```

The `-v $(pwd):/home/roxip/roxip` flag mounts your current directory into the
container, so the built artifacts (`roxip.bin`, `roxip.img`, etc.) appear on
your host after the container exits.

### 3. Run in QEMU

QEMU inside Docker needs privileged access:

```bash
# VersatilePB
docker run --rm -it --privileged -v $(pwd):/home/roxip/roxip roxip make run

# RISC-V virt (with OpenSBI)
docker run --rm -it --privileged -v $(pwd):/home/roxip/roxip roxip make riscv-virt run
```

Press **Ctrl+C** to stop QEMU.

## Using Docker Compose

If you have `docker-compose` installed, you can use the predefined services:

```bash
# Build all board images
docker-compose build

# Build a specific board
docker-compose run versatilepb
docker-compose run rockchip
docker-compose run amlogic
docker-compose run riscv-virt

# Build and run in QEMU (needs --privileged)
docker-compose run --privileged qemu
```

## Build Arguments

You can override the default board at build time:

```bash
docker build --build-arg BOARD=rockchip -t roxip-rockchip .
docker run --rm -it -v $(pwd):/home/roxip/roxip roxip-rockchip make img
```

## Output Files

After building, you'll find these artifacts in your project directory:

| File | Description |
|------|-------------|
| `roxip.elf` | ELF kernel image |
| `roxip.bin` | Raw binary (boots from `0x10000` on ARM, `0x80000000` on RISC-V) |
| `roxip.img` | Monolithic system image (kernel embedded + storage) |
| `roxip.storage` | 64 MB YAFS storage file |
| `roxip-cli` | Host CLI tool (if `BUILD_CLI=1`) |

## Example: Full Workflow

```bash
# 1. Build image
docker build -t roxip .

# 2. Build VersatilePB image
docker run --rm -it -v $(pwd):/home/roxip/roxip roxip make img

# 3. List output files
ls -la roxip.bin roxip.img roxip.storage

# 4. Run in QEMU
docker run --rm -it --privileged -v $(pwd):/home/roxip/roxip roxip make run
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| `docker: command not found` | Install Docker first |
| Permission denied on `/dev/kvm` | Add `--privileged` or add your user to the `kvm` group |
| Build artifacts owned by root | The container runs as user `roxip`; mounted volumes should match your host UID |
| QEMU fails to start | Ensure KVM is enabled: `ls /dev/kvm` should exist |

## License

RoxiP is released under the GPL. See `LICENSE` for details.
