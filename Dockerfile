# Dockerfile - RoxiP bare-metal OS build environment (Alpine Linux)
#
# Builds RoxiP for ARM (VersatilePB, Rockchip, Amlogic) and RISC-V (virt) boards
# using Meson + Ninja inside a container.
#
# Build for a specific architecture to avoid downloading unnecessary packages:
#
#   # ARM boards (versatilepb, rockchip, amlogic)
#   docker build --build-arg TARGET_ARCH=arm -t roxip-arm .
#   docker run --rm -it -v $(pwd):/roxip roxip-arm make rockchip img
#
#   # RISC-V board (riscv-virt)
#   docker build --build-arg TARGET_ARCH=riscv -t roxip-riscv .
#   docker run --rm -it -v $(pwd):/roxip roxip-riscv make riscv-virt img
#
# Run in QEMU (requires --privileged or --device /dev/kvm):
#   docker run --rm -it --privileged -v $(pwd):/roxip roxip-arm make run

FROM alpine:3.19

ARG TARGET_ARCH=arm

# Avoid interactive prompts
ENV APK_ARGS="--no-cache"

# Install core build tools (common to all architectures)
RUN apk add ${APK_ARGS} \
      alpine-sdk \
      bash \
      coreutils \
      file \
      git \
      make \
      meson \
      ninja \
      python3 \
      which

# Install architecture-specific toolchain and emulator
RUN if [ "${TARGET_ARCH}" = "riscv" ]; then \
      apk add ${APK_ARGS} \
        riscv64-elf-gcc \
        riscv64-elf-binutils \
        qemu-system-riscv; \
    else \
      apk add ${APK_ARGS} \
        arm-none-eabi-gcc \
        qemu-system-arm; \
    fi

# Create a non-root user for building
RUN adduser -D -s /bin/bash roxip && \
    mkdir -p /home/roxip/roxip && \
    chown -R roxip:roxip /home/roxip

WORKDIR /home/roxip/roxip

# Copy source code (respects .dockerignore)
COPY --chown=roxip:roxip . .

# Default command
CMD ["bash"]

# Environment variables for the build
ENV TARGET_ARCH=${TARGET_ARCH}
