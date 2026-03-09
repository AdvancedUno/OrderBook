#pragma once

#include "Order.h"
#include "MemoryPool.h"
#include <array>
#include <iostream>

namespace lob {

constexpr std::size_t MAX_ORDERS = 1'000'000;
constexpr std::size_t MAX_PRICES = 100'000;

class LimitOrderBook {
private:
    MemoryPool<Order> order_pool_{MAX_ORDERS};
    std::array<Order*, MAX_PRICES> bids_{nullptr};
    std::array<Order*, MAX_PRICES> asks_{nullptr};
    std::array<Order*, MAX_ORDERS> order_map_{nullptr};

public:
    LimitOrderBook() = default;

    // DECLARATION ONLY: The actual logic is in src/OrderBook.cpp
    void process_message(const MarketMessage& msg);

private:

    void match_buy_order(MarketMessage msg);
    void match_sell_order(MarketMessage msg);

    // Hot Path: Adding a new order to the book (Inline for speed)
    void add_limit_order(const MarketMessage& msg) {
        Order* new_order = order_pool_.allocate(msg.order_id, msg.price, msg.quantity, msg.side);
        if (!new_order) return; 

        order_map_[msg.order_id] = new_order;

        Order** head = (msg.side == Side::BUY) ? &bids_[msg.price] : &asks_[msg.price];

        if (*head == nullptr) {
            *head = new_order;
        } else {
            Order* current = *head;
            while (current->next != nullptr) {
                current = current->next;
            }
            current->next = new_order;
            new_order->prev = current;
        }
    }

    // Hot Path: Canceling an existing order (Inline for speed)
    void cancel_order(uint64_t order_id) {
        Order* target = order_map_[order_id];
        if (!target) return; 

        if (target->prev) {
            target->prev->next = target->next;
        } else {
            if (target->side == Side::BUY) {
                bids_[target->price] = target->next;
            } else {
                asks_[target->price] = target->next;
            }
        }

        if (target->next) {
            target->next->prev = target->prev;
        }

        order_map_[order_id] = nullptr;
        order_pool_.deallocate(target);
    }
};

} // namespace lob