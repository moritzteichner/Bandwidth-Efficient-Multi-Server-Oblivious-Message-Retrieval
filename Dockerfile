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
    m4 \
    libtool-bin \
    libntl-dev \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && \
    apt-get clean

# Set up work directory
WORKDIR /usr/src/

# Clone and build dpf-cpp
RUN git clone https://github.com/dkales/dpf-cpp.git && \
    mkdir -p dpf-cpp/build && \
    cd dpf-cpp/build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make

# Set work directory for your project
WORKDIR /usr/src/app
COPY . /usr/src/app

RUN git clone https://github.com/abb-iss/ophelib.git && \
    cd ophelib && \
    mkdir build && cd build && \
    cmake .. && \
    make -j && \
    make install

# Command to build your project
CMD ["cmake", "-B", "build", "-S", ".", "&&", "cmake", "--build", "build"]
