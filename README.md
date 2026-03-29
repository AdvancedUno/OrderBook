# Low Latency Limit Order Book

A high-performance, multi-threaded Limit Order Book (LOB) and matching engine written in C++17. Designed for high-frequency trading (HFT) environments, this engine avoids all kernel-level locks and dynamic memory allocations on the critical path to achieve nanosecond-scale latencies.

## Performance

In a simulated benchmark utilizing thread-pinning across two CPU cores, the engine achieves:
* **Throughput:** ~3.04 Million Orders / Second
* **Total Time for 500k Orders:** 164 ms

## Architecture & Mechanism Description

The system is composed of three primary low-latency mechanisms working in tandem:

### 1. Lock-Free SPSC Ring Buffer
To facilitate ultra-fast communication between the network/producer thread and the matching engine thread, the system uses a custom Single-Producer Single-Consumer (SPSC) Ring Buffer. 
* **Mechanism:** It utilizes `std::atomic` variables with strict memory ordering (`std::memory_order_acquire` and `std::memory_order_release`) to pass `MarketMessage` structs without mutex locks.
* **Optimization:** The buffer is explicitly aligned to the 64-byte cache line boundary (`alignas(64)`) to eliminate false sharing between CPU cores.

### 2. Custom Memory Pool (Slab Allocator)
Standard heap allocations (`new`/`delete`) are too slow for an HFT critical path. 
* **Mechanism:** At startup, the engine pre-allocates a massive contiguous block of memory for millions of `Order` objects. 
* **Optimization:** It uses an intrusive singly-linked free list and "placement new" to construct objects in $O(1)$ time, guaranteeing that the engine never asks the OS for memory during live trading.

### 3. O(1) Matching Engine
The core `LimitOrderBook` is optimized for continuous matching and rapid cancellations.
* **Mechanism:** Price levels are tracked using flat C-style arrays (`std::array`) rather than hash maps to ensure deterministic memory access.
* **Optimization:** Orders within a price level are stored as an intrusive doubly-linked list. When an order is filled or explicitly canceled, it unlinks itself in $O(1)$ time without requiring a traversal of the price level. The engine also maintains `best_bid` and `best_ask` pointers to avoid $O(N)$ scans of empty price levels during matching.

## Building and Running

### Prerequisites
* CMake
* A modern C++17 compiler GCC
* A Linux environment (for `pthread` thread-affinity features)


### Build Instructions
```bash
mkdir build
cd build
cmake ..
make
```

### Run the Engine

```bash
./hft_engine
```
