#pragma once

#include <cstdint>
#include <iostream>

namespace lob {

// We explicitly set the underlying type to uint8_t (1 byte) instead of the default int (4 bytes)
// to pack our structs as tightly as possible.
enum class Side : uint8_t {
    BUY = 0,
    SELL = 1
};

enum class OrderType : uint8_t {
    LIMIT = 0,
    MARKET = 1,
    CANCEL = 2
};

// The primary message sent from the Producer (Network) to the Consumer (Order Book)
// We order the members from largest to smallest to avoid implicit compiler padding.
struct alignas(8) MarketMessage {
    uint64_t order_id;      // 8 bytes
    uint64_t price;         // 8 bytes
    uint32_t quantity;      // 4 bytes
    Side side;              // 1 byte
    OrderType type;         // 1 byte
    uint8_t padding[2];     // 2 bytes of explicit padding to align the struct to 24 bytes total
    
    // Default constructor
    MarketMessage() = default;

    // Parameterized constructor
    MarketMessage(uint64_t id, uint64_t p, uint32_t q, Side s, OrderType t)
        : order_id(id), price(p), quantity(q), side(s), type(t) {}
};

// The node that will actually live inside the Limit Order Book's memory.
// It contains pointers to create an intrusive doubly-linked list for O(1) deletions.
struct Order {
    uint64_t order_id;
    uint64_t price;
    uint32_t quantity;
    Side side;
    
    // Pointers for the intrusive linked list within a specific price level
    Order* next{nullptr};
    Order* prev{nullptr};

    Order() = default;
    Order(uint64_t id, uint64_t p, uint32_t q, Side s)
        : order_id(id), price(p), quantity(q), side(s) {}
};

}