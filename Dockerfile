# Use a C++ development base image
FROM ubuntu:22.04

# Set non-interactive mode for apt
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libgmp-dev \
    libssl-dev \
    libgsl-dev \
    wget \
    gpg \
    gnupg2 \
    lsb-release \
    ca-certificates \
    build-essential \
    curl \
    m4 \
    libtool-bin \
    libntl-dev \
    && rm -rf /var/lib/apt/lists/*

# Install nasm
RUN apt-get update && \
    apt-get install -y nasm && \
    apt-get clean

# Update package list again to include Intel repository
RUN apt update

# Set up work directory
WORKDIR /usr/src/

# Clone and build dpf-cpp
RUN git clone https://github.com/dkales/dpf-cpp.git && \
    mkdir -p dpf-cpp/build && \
    cd dpf-cpp/build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make

WORKDIR /usr/src/

# Set work directory for your project
WORKDIR /usr/src/app
COPY . /usr/src/app

RUN cd ophelib && \
mkdir build && cd build && \
cmake .. && \
make -j && \
make install

# Command to build your project
CMD ["cmake", "-B", "build", "-S", ".", "&&", "cmake", "--build", "build"]
