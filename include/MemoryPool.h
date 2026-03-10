#pragma once

#include <vector>
#include <cstddef>
#include <utility>

namespace lob {

template <typename T>
class MemoryPool {
    
private:
    // We use a union-like approach via reinterpret_cast.
    // We force the block of memory to be exactly the size and alignment of our type T (the Order struct).
    struct alignas(alignof(T)) Block {
        std::byte data[sizeof(T)];
    };

    // We use std::vector to allocate the memory on the heap at startup.
    // We do not want this on the stack, or a 10-million order pool will cause a Stack Overflow.
    std::vector<Block> pool_;
    
    // Pointer to the top of our free list stack.
    void* free_head_;

public:
    // Constructor runs ONLY at startup. 
    // It allocates the memory and links every block together into a chain.
    explicit MemoryPool(std::size_t capacity) : pool_(capacity) {
        if (capacity == 0) {
            free_head_ = nullptr;
            return;
        }

        // Initialize the intrusive free list.
        // We take the first 8 bytes of the uninitialized block and store the address of the next block.
        for (std::size_t i = 0; i < capacity - 1; ++i) {
            void** current = reinterpret_cast<void**>(&pool_[i]);
            *current = &pool_[i + 1];
        }
        
        // The last block points to nullptr (end of the list)
        void** last = reinterpret_cast<void**>(&pool_[capacity - 1]);
        *last = nullptr;
        
        free_head_ = &pool_[0];
    }

    // Delete copy/move semantics to prevent accidentally duplicating massive memory blocks
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // Called on the Critical Path: Allocates an object in O(1) time
    template <typename... Args>
    T* allocate(Args&&... args) {
        if (!free_head_) {
            return nullptr; // Pool is completely exhausted
        }
        
        // 1. Pop the top block off the free list
        void* raw_memory = free_head_;
        free_head_ = *reinterpret_cast<void**>(free_head_);

        // 2. Placement New: Construct the object directly in our pre-allocated memory block.
        // This explicitly calls the constructor of T without asking the OS for memory.
        return new (raw_memory) T(std::forward<Args>(args)...);
    }

    // Called on the Critical Path: Returns an object to the pool in O(1) time
    void deallocate(T* ptr) {
        if (!ptr) return;
        
        // 1. Manually call the destructor since we used placement new.
        ptr->~T();

        // 2. Push the block back onto the top of the free list stack
        void** node = reinterpret_cast<void**>(ptr);
        *node = free_head_;
        free_head_ = ptr;
    }
};

} 