# Bandwidth-Efficient Multi-Server Oblivious Message Retrieval

This repository includes an implementation of a multi-server Oblivious Message Retrieval scheme as well as a benchmarking environment.

## Installation

1. **OS**: We use Ubuntu 22.04 as our operating system.
2. **Install Dependencies**: Installs essential build tools and libraries: `build-essential`, `cmake`, `git`, `libgmp-dev`, `libssl-dev`, `libgsl-dev`. `m4`, `libtool-bin`, `libntl-dev`.
3. **Install DPF Library by Daniel Kales `dpf-cpp`** https://github.com/dkales/dpf-cpp
4. **Install Paillier Library by ABB `ophelib`** https://github.com/abb-iss/ophelib

## Docker

We provide a docker file that automatically sets up an environment and installs all dependencies.
Build and run the docker container by

1. docker build -t app .
2. docker run -it app bash
  
## Build

Having everything installed, the project can be built as follows.

1. mkdir build
2. cd build
3. cmake ..
4. make

## Run

After having built the project, call the "run" binary with flags to run the simuation of our OMR protocol.
The flags are.

1. **N** - Number of messages
2. **M** - (Log) Number of users
3. **K** - Number of pertinent messages
4. **R** - Retrieval enabled (Binary value)
5. **B** - Benchmarking (Binary value)

For example: ./run -N 65000 -M 22 -K 50

