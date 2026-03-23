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

    // NEW: Track the top of the book to avoid O(N) scans
    uint64_t best_bid_{0};
    uint64_t best_ask_{MAX_PRICES};

public:
    LimitOrderBook() = default;

    void process_message(const MarketMessage& msg);

private:
    void match_buy_order(MarketMessage msg);
    void match_sell_order(MarketMessage msg);

    // Hot Path: Adding a new order
    void add_limit_order(const MarketMessage& msg) {
        Order* new_order = order_pool_.allocate(msg.order_id, msg.price, msg.quantity, msg.side);
        if (!new_order) return; 

        order_map_[msg.order_id] = new_order;

        if (msg.side == Side::BUY) {
            // Update best bid
            if (msg.price > best_bid_) best_bid_ = msg.price;
            
            Order** head = &bids_[msg.price];
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
        } else {
            // Update best ask
            if (msg.price < best_ask_) best_ask_ = msg.price;
            
            Order** head = &asks_[msg.price];
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
    }

    // ORIGINAL: Canceling via ID (Used by explicit Cancel messages)
    void cancel_order(uint64_t order_id) {
        Order* target = order_map_[order_id];
        cancel_order(target); // Delegate to the fast-path version
    }

    // NEW: Fast-Path Cancel (Used internally during matching)
    // Skips the order_map_ lookup since we already have the pointer
    void cancel_order(Order* target) {
        if (!target) return; 

        if (target->prev) {
            target->prev->next = target->next;
        } else {
            // If it's the head of the list, update the array pointer
            if (target->side == Side::BUY) {
                bids_[target->price] = target->next;
            } else {
                asks_[target->price] = target->next;
            }
        }

        if (target->next) {
            target->next->prev = target->prev;
        }

        order_map_[target->order_id] = nullptr;
        order_pool_.deallocate(target);
    }
};
}