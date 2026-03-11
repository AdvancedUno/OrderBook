#pragma once

#include <atomic>
#include <cstddef>
#include <array>

namespace lob {

// Standard cache line size for modern x86_64 architectures
constexpr std::size_t CACHE_LINE_SIZE = 64;

template <typename T, std::size_t Capacity>
class SPSCRingBuffer {
    static_assert((Capacity != 0) && ((Capacity & (Capacity - 1)) == 0), 
                  "Capacity must be a power of 2 for bitwise modulo optimization.");

private:
    constexpr static std::size_t MASK = Capacity - 1;

    alignas(CACHE_LINE_SIZE) std::array<T, Capacity> buffer_;
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> write_index_{0};
    alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> read_index_{0};

public:
    SPSCRingBuffer() = default;
    SPSCRingBuffer(const SPSCRingBuffer&) = delete;
    SPSCRingBuffer& operator=(const SPSCRingBuffer&) = delete;

    bool push(const T& item) {
        const std::size_t current_write = write_index_.load(std::memory_order_relaxed);
        const std::size_t next_write = (current_write + 1) & MASK;

        if (next_write == read_index_.load(std::memory_order_acquire)) {
            return false; 
        }

        buffer_[current_write] = item;
        write_index_.store(next_write, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        const std::size_t current_read = read_index_.load(std::memory_order_relaxed);

        if (current_read == write_index_.load(std::memory_order_acquire)) {
            return false; 
        }

        item = buffer_[current_read];
        read_index_.store((current_read + 1) & MASK, std::memory_order_release);
        return true;
    }
};

}