FROM mcr.microsoft.com/devcontainers/base:focal

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
    build-essential \
    g++-arm-linux-gnueabi \
    gcc-arm-linux-gnueabi \
    gcc-arm-none-eabi \
    gcc \
    libssl-dev \
    python3-dev \
    python3-pip \
    python3-venv \
    swig \
    telnet \
    unzip \
    usbutils \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*
