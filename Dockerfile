# Dockerfile - RoxiP bare-metal OS build environment
#
# Builds RoxiP for ARM (VersatilePB, Rockchip, Amlogic) and RISC-V (virt) boards
# using Meson + Ninja inside a container.
#
# Usage:
#   docker build -t roxip .
#   docker run --rm -it -v $(pwd):/roxip roxip
#
# Build a specific board:
#   docker build --build-arg BOARD=rockchip -t roxip-rockchip .
#   docker run --rm -it -v $(pwd):/roxip roxip-rockchip make rockchip img
#
# Run in QEMU (requires --privileged or --device /dev/kvm):
#   docker run --rm -it --privileged -v $(pwd):/roxip roxip make run

FROM archlinux:latest

# Avoid interactive prompts during package installation
ENV PACMAN_ARGS="--noconfirm"
ENV BOARD=versatilepb
ENV BUILD_CLI=0

# Install build dependencies
RUN pacman -Syu ${PACMAN_ARGS} && \
    pacman -S ${PACMAN_ARGS} \
      base-devel \
      git \
      meson \
      ninja \
      arm-none-eabi-gcc \
      riscv64-elf-gcc \
      riscv64-elf-binutils \
      qemu-system-arm \
      qemu-system-riscv \
      arch-install-scripts && \
    pacman -Scc ${PACMAN_ARGS}

# Create a non-root user for building (avoids permission issues with mounted volumes)
RUN useradd -m -s /bin/bash roxip && \
    mkdir -p /home/roxip/roxip && \
    chown -R roxip:roxip /home/roxip

WORKDIR /home/roxip/roxip

# Copy source code (respects .dockerignore)
COPY --chown=roxip:roxip . .

# Default command: show help
CMD ["bash"]

# Build arguments
ARG BOARD=versatilepb
ARG BUILD_CLI=0

# Environment variables for the build
ENV BOARD=${BOARD}
ENV BUILD_CLI=${BUILD_CLI}
