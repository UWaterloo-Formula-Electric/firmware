# Base: small Ubuntu with build tools available
FROM mcr.microsoft.com/devcontainers/base:ubuntu

ARG DEBIAN_FRONTEND=noninteractive
ARG ARM_GCC_VERSION=14.2.rel1 
# This is the latest version as of Sept 2025 that still supports Mac on X86_64 chips.

# 1) system tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl git make cmake python3 python3-pip \
    libusb-1.0-0 udev openocd gdb-multiarch \
    python3 python3-pip python3-venv \
 && rm -rf /var/lib/apt/lists/*

# 2) Arm GNU Toolchain  Installation
RUN set -eux; \
    arch="$(dpkg --print-architecture)"; \
    case "$arch" in \
      amd64)  url="https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_GCC_VERSION}/binrel/arm-gnu-toolchain-${ARM_GCC_VERSION}-x86_64-arm-none-eabi.tar.xz" ;; \
      arm64)  url="https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_GCC_VERSION}/binrel/arm-gnu-toolchain-${ARM_GCC_VERSION}-aarch64-arm-none-eabi.tar.xz" ;; \
      *) echo "Unsupported arch: $arch" && exit 1 ;; \
    esac; \
    curl -fsSL "$url" -o /tmp/arm-gcc.tar.xz; \
    mkdir -p /opt/arm-gcc; \
    tar -xJf /tmp/arm-gcc.tar.xz -C /opt/arm-gcc --strip-components=1; \
    rm -f /tmp/arm-gcc.tar.xz
ENV PATH="/opt/arm-gcc/bin:${PATH}"

# 3) Create a project venv baked into the image
ENV VIRTUAL_ENV=/opt/venv
ENV PATH="$VIRTUAL_ENV/bin:${PATH}"
ENV PIP_DISABLE_PIP_VERSION_CHECK=1 PYTHONDONTWRITEBYTECODE=1

# 4) Install Python deps from your repo's requirements.txt with caching
WORKDIR /tmp/py
COPY common/requirements.txt ./requirements.txt
RUN --mount=type=cache,target=/root/.cache/pip \
    python3 -m venv "$VIRTUAL_ENV" && \
    python -m pip install --upgrade pip && \
    python -m pip install -r requirements.txt

# 5) Add git safe directory for codespaces / devcontainer use. Otherwise, the build will complain
RUN git config --system --add safe.directory /workspaces/* 

# 6) Tiny helper: flash <boardName> → make load LOAD_TARGET=<boardName>
# Create a bin in PATH that calls `make load LOAD_TARGET=<boardName>`.
RUN printf '%s\n' '#!/usr/bin/env bash' \
  'set -euo pipefail' \
  'if [ $# -eq 0 ]; then' \
  '  echo "Usage: flash <boardName>" >&2; exit 1; fi' \
  'exec make load LOAD_TARGET="$1"' \
  > /usr/local/bin/flash && chmod +x /usr/local/bin/flash

# Useful defaults
ENV LC_ALL=C.UTF-8 LANG=C.UTF-8