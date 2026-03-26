FROM ubuntu:22.04 AS base

ENV DEBIAN_FRONTEND=noninteractive

FROM base AS requiredforstm

# Update CMake to the latest version
RUN apt-get update && \
    apt-get install -y wget && \
    wget -qO- "https://github.com/Kitware/CMake/releases/download/v3.27.9/cmake-3.27.9-linux-x86_64.sh" > /tmp/cmake.sh && \
    bash /tmp/cmake.sh --skip-license --prefix=/usr/local && \
    rm /tmp/cmake.sh

# Install build dependencies for OpenOCD and general tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-pip \
    make \
    xz-utils \
    tar \
    pkg-config \
    libtool \
    autoconf \
    automake \
    texinfo \
    libusb-1.0-0-dev \
    libftdi1-dev \
    libhidapi-dev \
    libjaylink-dev \
    libftdi1-dev \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
    build-essential \
    cppcheck \
    clang-tidy \
    && rm -rf /var/lib/apt/lists/*

# Build jimTcl
RUN git clone https://github.com/msteveb/jimtcl.git /tmp/jimtcl && \
    cd /tmp/jimtcl && \
    ./configure && make -j$(nproc) && make install && \
    rm -rf /tmp/jimtcl

# Build and install OpenOCD from source (latest master)
RUN git clone --recursive https://github.com/openocd-org/openocd.git /tmp/openocd && \
    cd /tmp/openocd && \
    ./bootstrap && \
    ./configure --enable-stlink --enable-cmsis-dap --disable-werror && \
    make -j$(nproc) && \
    make install && \
    rm -rf /tmp/openocd

# Install ARM GNU Toolchain
RUN wget -qO- https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz \
    | tar -xJ -C /opt && \
    ln -s /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gcc /usr/bin/arm-none-eabi-gcc && \
    ln -s /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-g++ /usr/bin/arm-none-eabi-g++ && \
    ln -s /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-ld /usr/bin/arm-none-eabi-ld && \
    ln -s /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-objcopy /usr/bin/arm-none-eabi-objcopy && \
    ln -s /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-objdump /usr/bin/arm-none-eabi-objdump && \
    ln -s /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-nm /usr/bin/arm-none-eabi-nm && \
    ln -s /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-size /usr/bin/arm-none-eabi-size && \
    ln -s /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-ar /usr/bin/arm-none-eabi-ar && \
    ln -s /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-as /usr/bin/arm-none-eabi-as && \
    ln -s /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gdb /usr/bin/arm-none-eabi-gdb && \
    ln -s /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-ranlib /usr/bin/arm-none-eabi-ranlib && \
    ln -s /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-strip /usr/bin/arm-none-eabi-strip

FROM requiredforstm AS createworkspace

WORKDIR /workspaces

FROM createworkspace AS customlibraries

# Trice
# Download and install trice
RUN mkdir -p /opt/trice && \
    wget -qO- https://github.com/rokath/trice/releases/download/v1.1.1/trice_linux_amd64.tar.gz \
    | tar -xz -C /opt/trice --strip-components=1 && \
    cd /opt/trice && \
    mv trice /usr/local/bin/trice && \
    chmod +x /usr/local/bin/trice

ENV PATH="/usr/local/bin:${PATH}"
RUN trice ver

FROM customlibraries AS createuser

# Add VSCode user
RUN useradd -ms /bin/bash vscode && \
    echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN chown -R vscode:vscode /workspaces
USER vscode