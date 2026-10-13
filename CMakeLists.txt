cmake_minimum_required(VERSION 3.15)

# Project name
project(MyCppProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1 -maes -mavx -mavx2 -Ofast -march=native")

# Include the library's headers
include_directories(/usr/src/dpf-cpp)
include_directories(/usr/include)

link_directories(/usr/lib/x86_64-linux-gnu)

# Add the library
add_library(dpf_lib
    /usr/src/dpf-cpp/AES.cpp
    /usr/src/dpf-cpp/Defines.cpp
    /usr/src/dpf-cpp/Log.cpp
    /usr/src/dpf-cpp/PRNG.cpp
    #/usr/src/dpf-cpp/bench.cpp
    /usr/src/dpf-cpp/dpf.cpp
    #/usr/src/dpf-cpp/hashdatastore.cpp
    #/usr/src/dpf-cpp/main.cpp
)

find_package(GSL REQUIRED)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/cmake")
find_package(OPHELib REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenMP)

# Add the executable
add_executable(run src/main.cpp)

# Link the library to your executable
target_link_libraries(run dpf_lib)
target_link_libraries(run gmp gmpxx)
target_link_libraries(run OPHELib::ophelib)
target_link_libraries(run GSL::gsl GSL::gslcblas)
