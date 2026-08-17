#ifndef ORDERPOOL_H
#define ORDERPOOL_H

#include "BuyOrder.h"
#include "SellOrder.h"
#include <vector>
#include <cstddef>

// Pre-allocated Ring Buffer Memory Pool for Orders
// Goal: Zero dynamic heap allocations (no new/delete) during trading execution.
class OrderPool {
private:
    std::vector<BuyOrder> buyPool;   // Contiguous block of pre-built Buy Orders
    std::vector<SellOrder> sellPool; // Contiguous block of pre-built Sell Orders

    size_t buyIndex = 0;   // Points to the next available slot in buyPool
    size_t sellIndex = 0;  // Points to the next available slot in sellPool
    size_t capacity;       // Maximum size of the pool

public:
    // Constructor: Allocates all memory upfront at setup phase
    explicit OrderPool(size_t cap) : capacity(cap) {
        // Step 1: Δεσμεύουμε την ακατέργαστη μνήμη χωρίς να φτιάξουμε αντικείμενα (Zero Copy)
        buyPool.reserve(capacity);
        sellPool.reserve(capacity);

        // Step 2: Κατασκευάζουμε το κάθε αντικείμενο ΕΝΑ-ΕΝΑ απευθείας στη μνήμη.
        // Έτσι ο static counter των IDs θα αυξάνεται κανονικά!
        for (size_t i = 0; i < capacity; ++i) {
            buyPool.emplace_back(0.0, 0);
            sellPool.emplace_back(0.0, 0);
        }
    }

    // Fast O(1) allocation for Buy Orders (Zero Heap Overhead)
    BuyOrder* AllocateBuy(double price, int qty) {
        BuyOrder* order = &buyPool[buyIndex];
        buyIndex = (buyIndex + 1) % capacity; // Ring buffer wrap-around

        order->SetPrice(price);
        order->SetQuantity(qty);
        return order;
    }

    // Fast O(1) allocation for Sell Orders (Zero Heap Overhead)
    SellOrder* AllocateSell(double price, int qty) {
        SellOrder* order = &sellPool[sellIndex];
        sellIndex = (sellIndex + 1) % capacity; // Ring buffer wrap-around

        order->SetPrice(price);
        order->SetQuantity(qty);
        return order;
    }

    // Reset pointers to start recycling memory without calling vector clear or destructors
    void Reset() {
        buyIndex = 0;
        sellIndex = 0;
    }
};

#endif // ORDERPOOL_H