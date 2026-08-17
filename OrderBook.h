#ifndef UNTITLED6_ORDERBOOK_H
#define UNTITLED6_ORDERBOOK_H

#include <vector>
#include <iostream>
#include <algorithm>
#include "Order.h"

// Represents a single executed transaction between two market participants
struct alignas(64) TradeMatch {
    int buyerId;
    int sellerId;
    double price;
    int quantity;
};

class OrderBook {
private:
    // Fixed-size Array parameters for Direct Indexing
    static constexpr size_t MAX_PRICE_LEVELS = 10000;
    static constexpr double BASE_PRICE = 100.00;
    static constexpr size_t MAX_ORDERS = 100000;

    // 1. The Core Price Level Array (Replaces std::map)
    alignas(64) std::vector<Order*> priceArray[MAX_PRICE_LEVELS];

    std::vector<Order*> orderRegistry;

    // Bounds for fast sweeping
    int minActiveIndex = MAX_PRICE_LEVELS;
    int maxActiveIndex = -1;

    // Converts a limit price to a fixed array index in O(1) time
    inline int PriceToIndex(double price) const {
        int index = static_cast<int>((price - BASE_PRICE) * 100.0);
        if (index < 0) return 0;
        if (index >= static_cast<int>(MAX_PRICE_LEVELS)) return MAX_PRICE_LEVELS - 1;
        return index;
    }

    // Validation using raw pointers
    bool ValidateOrder(const Order* order) const;

public:
    bool silentMode = false;
    std::vector<TradeMatch> tradeLedger;

    OrderBook();

    // Core Exchange Operations (Now using ONLY raw pointers)
    void CancelOrder(int id);
    void InsertOrder(Order* newOrder);
    int GetVolumeAtLevel(double price);

    // Our Single, Unified Matching Engine Function
    void MatchOrder(Order* newOrder);

    // Market Order Sweep
    void ExecuteMarketOrder(bool isBuySide, int requestedQty);

    // Utilities
    void PrintOrderBook() const;
    void PrintTradeHistory() const;
    void SetSilentMode(bool mode);
};

#endif // UNTITLED6_ORDERBOOK_H