#include <iostream>
#include <thread>
#include <chrono>
#include <pthread.h>
#include <sched.h> 

#include "SPSCRingBuffer.h"
#include "Order.h"

using namespace lob;

constexpr std::size_t BUFFER_CAPACITY = 1024;
constexpr std::size_t MESSAGES_TO_SEND = 10'000'000;

void pin_thread_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

void producer_thread(SPSCRingBuffer<MarketMessage, BUFFER_CAPACITY>& buffer) {
    pin_thread_to_core(1); 
    
    MarketMessage msg;
    msg.type = OrderType::LIMIT;
    msg.side = Side::BUY;
    msg.quantity = 100;
    msg.price = 50000;

    for (uint64_t i = 0; i < MESSAGES_TO_SEND; ++i) {
        msg.order_id = i;
        while (!buffer.push(msg)) {} // Spin-wait
    }
}

void consumer_thread(SPSCRingBuffer<MarketMessage, BUFFER_CAPACITY>& buffer, std::chrono::nanoseconds& total_duration) {
    pin_thread_to_core(2); 

    MarketMessage msg;
    uint64_t messages_received = 0;
    
    auto start_time = std::chrono::high_resolution_clock::now();

    while (messages_received < MESSAGES_TO_SEND) {
        if (buffer.pop(msg)) {
            messages_received++;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    total_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
}

int main() {
    std::cout << "Starting Lock-Free SPSC Ring Buffer Benchmark...\n";

    SPSCRingBuffer<MarketMessage, BUFFER_CAPACITY> ring_buffer;
    std::chrono::nanoseconds total_duration(0);

    std::thread consumer(consumer_thread, std::ref(ring_buffer), std::ref(total_duration));
    std::thread producer(producer_thread, std::ref(ring_buffer));

    producer.join();
    consumer.join();

    double seconds = total_duration.count() / 1e9;
    double ops_per_second = MESSAGES_TO_SEND / seconds;
    double avg_nanoseconds_per_msg = static_cast<double>(total_duration.count()) / MESSAGES_TO_SEND;

    std::cout << "Successfully processed " << MESSAGES_TO_SEND << " messages.\n";
    std::cout << "Total Time: " << seconds << " seconds\n";
    std::cout << "Throughput: " << ops_per_second / 1e6 << " Million Ops/Sec\n";
    std::cout << "Average Latency: " << avg_nanoseconds_per_msg << " ns/message\n";

    return 0;
}