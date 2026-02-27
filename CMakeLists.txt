cmake_minimum_required(VERSION 3.10)
project(UltraLowLatencyLOB VERSION 1.0 LANGUAGES CXX)

# Enforce C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Aggressive optimizations for the whole project
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -pthread")

# Add the include directory so the compiler can find your headers
include_directories(include)

# (Later) Build the main LOB library
# add_library(lob_core src/OrderBook.cpp)

# Build the Ring Buffer Benchmark
add_executable(benchmark_ringbuffer benchmarks/benchmark_ringbuffer.cpp)
target_link_libraries(benchmark_ringbuffer pthread)