#ifndef ORDERPOOL_H
#define ORDERPOOL_H

#include "BuyOrder.h"
#include "SellOrder.h"
#include <vector>
#include <memory>

class OrderPool {
private:
    std::vector<std::unique_ptr<Order>> pool;
    size_t nextIndex = 0;

public:
    OrderPool(size_t capacity) {
        pool.reserve(capacity);
    }

    //  Fast O(1) allocation for Buy Orders
    Order* AllocateBuy(double price, int qty) {
        pool.push_back(std::make_unique<BuyOrder>(price, qty));
        return pool.back().get();
    }

    //  Fast O(1) allocation for Sell Orders
    Order* AllocateSell(double price, int qty) {
        pool.push_back(std::make_unique<SellOrder>(price, qty));
        return pool.back().get();
    }

    void Reset() {
        pool.clear();
        nextIndex = 0;
    }
};

#endif // ORDERPOOL_H