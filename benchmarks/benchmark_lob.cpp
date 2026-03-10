#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <memory>

#include "OrderBook.h"

using namespace lob;

constexpr std::size_t NUM_ORDERS = 1'000'000;

int main() {
    std::cout << "Starting Limit Order Book Matching Benchmark...\n";

    auto lob = std::make_unique<LimitOrderBook>();
    
    std::vector<MarketMessage> orders;
    orders.reserve(NUM_ORDERS);

    std::mt19937 rng(42); 
    std::uniform_int_distribution<uint64_t> price_dist(49900, 50100);
    std::uniform_int_distribution<uint32_t> qty_dist(10, 100);
    std::uniform_int_distribution<int> side_dist(0, 1); 

    for (std::size_t i = 0; i < NUM_ORDERS; ++i) {
        Side side = (side_dist(rng) == 0) ? Side::BUY : Side::SELL;
        orders.emplace_back(i, price_dist(rng), qty_dist(rng), side, OrderType::LIMIT);
    }

    std::cout << "Pre-generated " << NUM_ORDERS << " orders. Running engine...\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& msg : orders) {
        lob->process_message(msg);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    // ---------------------------------------------------------

    std::chrono::nanoseconds total_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

    double seconds = total_duration.count() / 1e9;
    double ops_per_second = NUM_ORDERS / seconds;
    double avg_nanoseconds_per_order = static_cast<double>(total_duration.count()) / NUM_ORDERS;

    std::cout << "Total Time: " << seconds << " seconds\n";
    std::cout << "Throughput: " << ops_per_second / 1e6 << " Million Orders/Sec\n";
    std::cout << "Average Latency: " << avg_nanoseconds_per_order << " ns/order\n";

    return 0;
}