# Base: small Ubuntu with build tools available
FROM mcr.microsoft.com/devcontainers/base:ubuntu

ARG DEBIAN_FRONTEND=noninteractive
ARG ARM_GCC_VERSION=14.3.rel1 
# This is the latest version as of Sept 2025

# 1) system tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl git make cmake python3 python3-pip \
    libusb-1.0-0 udev openocd gdb-multiarch \
    python3 python3-pip python3-venv \
    openocd gdb-multiarch \
 && rm -rf /var/lib/apt/lists/*

# 2) Arm GNU Toolchain  Installation
# -------------------------
# Arm GNU Toolchain (official tarball)
# -------------------------
# Downloads page shows current releases; pin a specific version for reproducibility.
# https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
RUN curl -L "https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_GCC_VERSION}/binrel/arm-gnu-toolchain-${ARM_GCC_VERSION}-x86_64-arm-none-eabi.tar.xz" \
    -o /tmp/arm-gcc.tar.xz \
 && mkdir -p /opt/arm-gcc \
 && tar -xJf /tmp/arm-gcc.tar.xz -C /opt/arm-gcc --strip-components=1 \
 && rm /tmp/arm-gcc.tar.xz
ENV PATH="/opt/arm-gcc/bin:${PATH}"

# 3) Create a project venv baked into the image
ENV VIRTUAL_ENV=/opt/venv
ENV PATH="$VIRTUAL_ENV/bin:${PATH}"
# optional niceties
ENV PIP_DISABLE_PIP_VERSION_CHECK=1 PYTHONDONTWRITEBYTECODE=1

# 4) Install Python deps from your repo's requirements.txt with caching
# NOTE: The build context is the repo root; this copy works even though the Dockerfile is in .devcontainer/
COPY common/requirements.txt /tmp/requirements.txt
RUN --mount=type=cache,target=/root/.cache/pip \
    python3 -m venv "${VIRTUAL_ENV}" && \
    python -m pip install --upgrade pip && \
    python -m pip install -r /tmp/requirements.txt

# 5) Add git safe directory for codespaces / devcontainer use. Otherwise, the build will complain
RUN git config --system --add safe.directory /workspaces/* 

# Flash helper: `flash <boardName>`. 
# Create a bin in PATH that calls `make load LOAD_TARGET=<boardName>`.
RUN printf '%s\n' '#!/usr/bin/env bash' \
  'set -euo pipefail' \
  'if [ $# -eq 0 ]; then' \
  '  echo "Usage: flash <boardName>" >&2; exit 1; fi' \
  'exec make load LOAD_TARGET="$1"' \
  > /usr/local/bin/flash && chmod +x /usr/local/bin/flash

# Useful defaults
ENV LC_ALL=C.UTF-8 LANG=C.UTF-8