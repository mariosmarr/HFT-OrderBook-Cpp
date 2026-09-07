#ifndef SPSCQUEUE_H
#define SPSCQUEUE_H

#include <atomic>
#include <cstddef>
#include "Order.h"

// Lock-free bounded SPSC queue for thread decoupling
template<typename T, size_t Capacity>
class SPSCQueue {
private:
    T buffer[Capacity];

    // Align to distinct cache lines to avoid false sharing between threads
    alignas(64) std::atomic<size_t> head{0}; // producer index
    alignas(64) std::atomic<size_t> tail{0}; // consumer index

public:
    SPSCQueue() = default;

    // Called only by producer (network ingestion thread)
    bool Push(const T& item) {
        size_t currentHead = head.load(std::memory_order_relaxed);
        size_t currentTail = tail.load(std::memory_order_acquire); // sync with consumer's tail update

        // queue is full
        if ((currentHead + 1) % Capacity == currentTail) {
            return false;
        }

        buffer[currentHead] = item;
        // Release store ensures payload write is visible before head index increments
        head.store((currentHead + 1) % Capacity, std::memory_order_release);
        return true;
    }

    // Called only by consumer (matching engine core)
    bool Pop(T& item) {
        size_t currentTail = tail.load(std::memory_order_relaxed);
        size_t currentHead = head.load(std::memory_order_acquire); // sync with producer's head update

        // queue is empty
        if (currentTail == currentHead) {
            return false;
        }

        item = buffer[currentTail];
        // Release store ensures buffer read finishes before slot is marked reusable
        tail.store((currentTail + 1) % Capacity, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool Empty() const {
        return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed);
    }
};

#endif // SPSCQUEUE_H