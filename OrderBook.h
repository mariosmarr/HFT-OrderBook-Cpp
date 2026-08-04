#ifndef UNTITLED6_ORDERBOOK_H
#define UNTITLED6_ORDERBOOK_H

#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>
#include "Order.h"
#include <map>

// Represents a single executed transaction between two market participants
struct alignas(64) TradeMatch {
    int buyerId;
    int sellerId;
    double price;
    int quantity;
};
class OrderBook {
private:
    // Fixed-size Array for Direct Indexing Optimization
    static constexpr size_t MAX_PRICE_LEVELS = 1000;
    static constexpr double BASE_PRICE = 100.00;

    // Array of vectors for contiguous memory layout
    alignas(64) std::vector<Order*> priceArray[MAX_PRICE_LEVELS];

    int minActiveIndex = MAX_PRICE_LEVELS;
    int maxActiveIndex = -1;
    // Converts a limit price to a fixed array index in O(1) time
    inline int PriceToIndex(double price) const {
        int index = static_cast<int>((price - BASE_PRICE) * 100.0);
        if (index < 0) return 0;
        if (index >= static_cast<int>(MAX_PRICE_LEVELS)) return MAX_PRICE_LEVELS - 1;
        return index;
    }

public:
    // High-frequency matching function using flat array structure
    void MatchFlatPooledOrder(Order* newOrder);

    bool silentMode = false;
    // Holds full ownership of all active orders for O(1) lifecycle management
    std::unordered_map<int, std::unique_ptr<Order>> orderMap;

    // Maps price levels to raw pointers of orders for fast matching and execution
    std::map<double, std::vector<Order*>> PriceMap;
    std::vector<TradeMatch> tradeLedger;

    // Helper method to validate incoming order specifications before processing
    bool ValidateOrder(const std::unique_ptr<Order>& order) const;

public:
    OrderBook();

    // Core Exchange Operations
    void CancelOrder(int id);
    void InsertOrder(std::unique_ptr<Order> newOrder);
    int GetVolumeAtLevel(double price);
    void MatchOrder(std::unique_ptr<Order> newOrder);
    void PrintOrderBook() const;
    void MatchPooledOrder(Order* newOrder);


    void ExecuteMarketOrder(bool isBuySide,int requestedQty);
    void PrintTradeHistory() const;
    void SetSilentMode(bool silentMode);
};

#endif // UNTITLED6_ORDERBOOK_H