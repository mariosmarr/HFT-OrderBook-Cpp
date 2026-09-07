#ifndef ORDERPOOL_H
#define ORDERPOOL_H

#include "BuyOrder.h"
#include "SellOrder.h"
#include <vector>
#include <cstddef>

// Pre-allocated ring buffer pool to avoid new/delete on the hot path
class OrderPool {
private:
    std::vector<BuyOrder> buyPool;   // contiguous storage for cache locality
    std::vector<SellOrder> sellPool;

    size_t buyIndex = 0;   // next available slot
    size_t sellIndex = 0;
    size_t capacity;

public:
    // Pre-allocate upfront during engine init
    explicit OrderPool(size_t cap) : capacity(cap) {
        buyPool.reserve(capacity);
        sellPool.reserve(capacity);

        // emplace in-place so constructors run and static IDs increment correctly
        for (size_t i = 0; i < capacity; ++i) {
            buyPool.emplace_back(0.0, 0);
            sellPool.emplace_back(0.0, 0);
        }
    }

    // O(1) alloc - recycle existing object
    BuyOrder* AllocateBuy(double price, int qty) {
        BuyOrder* order = &buyPool[buyIndex];
        buyIndex = (buyIndex + 1) % capacity; // ring buffer wrap-around

        order->SetPrice(price);
        order->SetQuantity(qty);
        return order;
    }

    // O(1) alloc - recycle existing object
    SellOrder* AllocateSell(double price, int qty) {
        SellOrder* order = &sellPool[sellIndex];
        sellIndex = (sellIndex + 1) % capacity; // ring buffer wrap-around

        order->SetPrice(price);
        order->SetQuantity(qty);
        return order;
    }

    // Rewind indices to reuse buffer without vector reallocations or destructors
    void Reset() {
        buyIndex = 0;
        sellIndex = 0;
    }
};

#endif // ORDERPOOL_H