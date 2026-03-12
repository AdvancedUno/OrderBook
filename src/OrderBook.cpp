#include "OrderBook.h"
#include <algorithm>

namespace lob {

void LimitOrderBook::process_message(const MarketMessage& msg) {
    if (msg.type == OrderType::CANCEL) {
        cancel_order(msg.order_id);
        return;
    }

    if (msg.side == Side::BUY) {
        match_buy_order(msg);
    } else {
        match_sell_order(msg);
    }
}

// Hot Path: Matching an incoming Buy order against resting Asks
void LimitOrderBook::match_buy_order(MarketMessage msg) {
    // OPTIMIZED: Start scanning directly from the best_ask_
    for (uint64_t price = best_ask_; price <= msg.price && msg.quantity > 0; ++price) {
        Order* resting_ask = asks_[price];

        while (resting_ask != nullptr && msg.quantity > 0) {
            uint32_t trade_qty = std::min(msg.quantity, resting_ask->quantity);
            
            msg.quantity -= trade_qty;
            resting_ask->quantity -= trade_qty;

            Order* next_ask = resting_ask->next;

            if (resting_ask->quantity == 0) {
                // OPTIMIZED: Pass the pointer directly to avoid array lookup
                cancel_order(resting_ask); 
            }

            resting_ask = next_ask;
        }
    }

    // NEW: Update best_ask_ if we fully cleared the top levels
    while (best_ask_ < MAX_PRICES && asks_[best_ask_] == nullptr) {
        best_ask_++;
    }

    if (msg.quantity > 0 && msg.type == OrderType::LIMIT) {
        add_limit_order(msg); 
    }
}

// Hot Path: Matching an incoming Sell order against resting Bids
void LimitOrderBook::match_sell_order(MarketMessage msg) {
    // OPTIMIZED: Start scanning directly from the best_bid_
    for (uint64_t price = best_bid_; price >= msg.price && msg.quantity > 0; --price) {
        Order* resting_bid = bids_[price];

        while (resting_bid != nullptr && msg.quantity > 0) {
            uint32_t trade_qty = std::min(msg.quantity, resting_bid->quantity);
            
            msg.quantity -= trade_qty;
            resting_bid->quantity -= trade_qty;

            Order* next_bid = resting_bid->next;

            if (resting_bid->quantity == 0) {
                // OPTIMIZED: Fast-path delete
                cancel_order(resting_bid);
            }

            resting_bid = next_bid;
        }
        
        if (price == 0) break; // Prevent underflow
    }

    // NEW: Update best_bid_ if we fully cleared the top levels
    // Careful with underflow here since it's an unsigned integer
    while (best_bid_ > 0 && bids_[best_bid_] == nullptr) {
        best_bid_--;
    }
    // Handle the 0 edge case
    if (best_bid_ == 0 && bids_[0] == nullptr) {
         // Book is empty of bids, leave best_bid_ at 0
    }

    if (msg.quantity > 0 && msg.type == OrderType::LIMIT) {
        add_limit_order(msg);
    }
}

}