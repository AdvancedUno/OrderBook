cmake_minimum_required(VERSION 3.10)

# Project Name and Language
project(UltraLowLatencyLOB VERSION 1.0 LANGUAGES CXX)

# Require C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ------------------------------------------------------------------------------
# HFT Compiler Flags
# -O3: Maximum optimization (inlining, loop unrolling, vectorization)
# -march=native: Optimizes the binary specifically for the CPU compiling it
# -DNDEBUG: Disables all assert() macros to remove branching in the critical path
# -pthread: Required for our CPU core pinning
# ------------------------------------------------------------------------------
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -DNDEBUG -pthread -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -pthread -Wall -Wextra")

# Include directories so our cpp files can find the headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# 1. Build the core Limit Order Book library
# This compiles the source file into a static library we can link against
add_library(lob_core src/OrderBook.cpp)

# 2. Build the Ring Buffer Benchmark executable
add_executable(benchmark_ringbuffer benchmarks/benchmark_ringbuffer.cpp)
target_link_libraries(benchmark_ringbuffer PRIVATE lob_core pthread)

# 3. Build the Order Book Latency Benchmark executable (Next Step)
add_executable(benchmark_lob benchmarks/benchmark_lob.cpp)
target_link_libraries(benchmark_lob PRIVATE lob_core pthread)


# 4. Build the Main HFT Engine Executable
add_executable(hft_engine src/main.cpp)
target_link_libraries(hft_engine PRIVATE lob_core pthread)