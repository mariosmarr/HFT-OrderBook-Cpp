#ifndef UNTITLED6_ORDERBOOK_H
#define UNTITLED6_ORDERBOOK_H

#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>
#include "Order.h"

class OrderBook {
private:
    // Holds full ownership of all active orders for O(1) lifecycle management
    std::unordered_map<int, std::unique_ptr<Order>> orderMap;

    // Maps price levels to raw pointers of orders for fast matching and execution
    std::unordered_map<double, std::vector<Order*>> PriceMap;

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
    void ExecuteMarketOrder(bool isBuySide,int requestedQty);
};

#endif // UNTITLED6_ORDERBOOK_H