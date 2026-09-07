#ifndef UNTITLED6_ORDERBOOK_H
#define UNTITLED6_ORDERBOOK_H

#include <vector>
#include <iostream>
#include <algorithm>
#include "Order.h"

// Trade execution record aligned to cache line boundary
struct alignas(64) TradeMatch {
    int buyerId;
    int sellerId;
    double price;
    int quantity;
};

class OrderBook {
private:
    // Fixed array indexing bounds
    static constexpr size_t MAX_PRICE_LEVELS = 10000;
    static constexpr double BASE_PRICE = 100.00;
    static constexpr size_t MAX_ORDERS = 100000;

    // Direct price-indexed array to avoid std::map red-black tree traversal
    alignas(64) std::vector<Order*> priceArray[MAX_PRICE_LEVELS];

    // Lookup index for O(1) order cancellation by ID
    std::vector<Order*> orderRegistry;

    // Active price boundary tracking for fast sweeps
    int minActiveIndex = MAX_PRICE_LEVELS;
    int maxActiveIndex = -1;

    // O(1) direct mapping from float price to contiguous array index (1 cent ticks)
    inline int PriceToIndex(double price) const {
        int index = static_cast<int>((price - BASE_PRICE) * 100.0);
        if (index < 0) return 0;
        if (index >= static_cast<int>(MAX_PRICE_LEVELS)) return MAX_PRICE_LEVELS - 1;
        return index;
    }

    bool ValidateOrder(const Order* order) const;

public:
    bool silentMode = false;
    std::vector<TradeMatch> tradeLedger;

    OrderBook();

    // Order operations (raw pointers managed via pre-allocated pool)
    void CancelOrder(int id);
    void InsertOrder(Order* newOrder);
    int GetVolumeAtLevel(double price);

    // Core matching engine loop (in-place matching against resting liquidity)
    void MatchOrder(Order* newOrder);

    // Aggressive sweeping across active price levels
    void ExecuteMarketOrder(bool isBuySide, int requestedQty);

    // Helpers
    void PrintOrderBook() const;
    void PrintTradeHistory() const;
    void SetSilentMode(bool mode);
};

#endif // UNTITLED6_ORDERBOOK_H