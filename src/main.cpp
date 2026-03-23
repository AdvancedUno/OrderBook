#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <pthread.h>
#include <sched.h> 

#include "OrderBook.h"
#include "SPSCRingBuffer.h"

using namespace lob;

constexpr std::size_t BUFFER_CAPACITY = 1024;
constexpr std::size_t MESSAGES_TO_PROCESS = 500'000;

// Helper to lock a thread to a specific CPU core
void pin_thread_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

// SIMULATED NETWORK THREAD (Future Crypto Data Pipeline)
void network_producer(SPSCRingBuffer<MarketMessage, BUFFER_CAPACITY>& buffer) {
    pin_thread_to_core(1); 
    std::cout << "[Producer] Network thread pinned to Core 1. Listening for data...\n";
    
    MarketMessage msg;
    msg.type = OrderType::LIMIT;
    msg.quantity = 100;

    // Simulate an incoming burst of orders
    for (uint64_t i = 0; i < MESSAGES_TO_PROCESS; ++i) {
        msg.order_id = i;
        msg.side = (i % 2 == 0) ? Side::BUY : Side::SELL;
        msg.price = 50000 + (i % 100); // Varied prices around $50k
        
        // Spin-wait if the buffer is full
        while (!buffer.push(msg)) {} 
    }
    
    std::cout << "[Producer] Finished receiving " << MESSAGES_TO_PROCESS << " messages.\n";
}

// MATCHING ENGINE THREAD
void matching_engine_consumer(SPSCRingBuffer<MarketMessage, BUFFER_CAPACITY>& buffer, LimitOrderBook& lob) {
    pin_thread_to_core(2); 
    std::cout << "[Consumer] Engine thread pinned to Core 2. Ready to match...\n";

    MarketMessage msg;
    uint64_t messages_processed = 0;
    
    auto start_time = std::chrono::high_resolution_clock::now();

    // Hot Loop: Continuously poll the ring buffer for new messages
    while (messages_processed < MESSAGES_TO_PROCESS) {
        if (buffer.pop(msg)) {
            lob.process_message(msg);
            messages_processed++;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "[Consumer] Processed " << messages_processed << " orders.\n";
    std::cout << "[Consumer] Total Matching Time: " << duration.count() << " ms\n";
}


int main() {
    std::cout << "--- Starting High-Frequency Trading Engine ---\n";

    // 1. Initialize the shared Lock-Free Ring Buffer
    SPSCRingBuffer<MarketMessage, BUFFER_CAPACITY> ring_buffer;

    // 2. Initialize the Limit Order Book ON THE HEAP
    auto lob = std::make_unique<LimitOrderBook>();

    // 3. Spin up the Producer and Consumer threads
    // Notice we dereference (*lob) to pass it by reference to the thread
    std::thread engine_thread(matching_engine_consumer, std::ref(ring_buffer), std::ref(*lob));
    std::thread network_thread(network_producer, std::ref(ring_buffer));

    // 4. Wait for both threads to finish their execution
    network_thread.join();
    engine_thread.join();

    std::cout << "--- Engine Shutdown Complete ---\n";
    return 0;
}