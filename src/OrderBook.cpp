#include "OrderBook.h"
#include <algorithm>

namespace lob {

// We implement the matching logic here to keep the header clean and 
// allow the compiler to optimize the translation unit effectively.

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
    // Start looking at the lowest possible price (0) up to the incoming buy price
    // In a real system, you would track the 'best_ask' index to avoid scanning from 0.
    for (uint64_t price = 0; price <= msg.price && msg.quantity > 0; ++price) {
        Order* resting_ask = asks_[price];

        // While there are orders at this price level and we still need to buy
        while (resting_ask != nullptr && msg.quantity > 0) {
            uint32_t trade_qty = std::min(msg.quantity, resting_ask->quantity);
            
            // Execute the trade (In a real system, we'd send an execution message here)
            // std::cout << "TRADE: " << trade_qty << " shares @ $" << price << "\n";

            msg.quantity -= trade_qty;
            resting_ask->quantity -= trade_qty;

            Order* next_ask = resting_ask->next;

            // If the resting order is fully filled, remove it from the book
            if (resting_ask->quantity == 0) {
                cancel_order(resting_ask->order_id); 
            }

            // Move to the next oldest order at this price level (Time Priority)
            resting_ask = next_ask;
        }
    }

    // If there is still quantity left after clearing the available Asks, 
    // the remainder becomes a resting Bid in the order book.
    if (msg.quantity > 0 && msg.type == OrderType::LIMIT) {
        add_limit_order(msg); // This calls the inline function we wrote in the header
    }
}

// Hot Path: Matching an incoming Sell order against resting Bids
void LimitOrderBook::match_sell_order(MarketMessage msg) {
    // Start looking at the highest possible price down to the incoming sell price
    // Note: To avoid integer underflow on uint64_t, we carefully check the bounds
    for (uint64_t price = MAX_PRICES - 1; price >= msg.price && msg.quantity > 0; --price) {
        Order* resting_bid = bids_[price];

        while (resting_bid != nullptr && msg.quantity > 0) {
            uint32_t trade_qty = std::min(msg.quantity, resting_bid->quantity);
            
            msg.quantity -= trade_qty;
            resting_bid->quantity -= trade_qty;

            Order* next_bid = resting_bid->next;

            if (resting_bid->quantity == 0) {
                cancel_order(resting_bid->order_id);
            }

            resting_bid = next_bid;
        }
        
        if (price == 0) break; // Prevent underflow
    }

    if (msg.quantity > 0 && msg.type == OrderType::LIMIT) {
        add_limit_order(msg);
    }
}

} 