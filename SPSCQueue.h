#ifndef SPSCQUEUE_H
#define SPSCQUEUE_H

#include <atomic>
#include <cstddef>
#include "Order.h"

// Lock-Free Single-Producer Single-Consumer (SPSC) Ring Buffer Queue
template<typename T, size_t Capacity>
class SPSCQueue {
private:
    T buffer[Capacity];

    // Align indices to separate 64-byte cache lines to prevent False Sharing
    alignas(64) std::atomic<size_t> head{0}; // Written by Producer (Network Thread)
    alignas(64) std::atomic<size_t> tail{0}; // Written by Consumer (Engine Thread)

public:
    SPSCQueue() = default;


    // Push item into queue (Producer Only)
    bool Push(const T& item) {
        size_t currentHead = head.load(std::memory_order_relaxed);
        size_t currentTail = tail.load(std::memory_order_acquire); // Hardware Barrier

        // Check buffer overflow
        if ((currentHead + 1) % Capacity == currentTail) {
            return false;
        }

        buffer[currentHead] = item;
        head.store((currentHead + 1) % Capacity, std::memory_order_release); // Hardware Barrier
        return true;
    }

    // Pop item from queue (Consumer Only)
    bool Pop(T& item) {
        size_t currentTail = tail.load(std::memory_order_relaxed);
        size_t currentHead = head.load(std::memory_order_acquire); // Hardware Barrier

        // Check buffer underflow
        if (currentTail == currentHead) {
            return false;
        }

        item = buffer[currentTail];
        tail.store((currentTail + 1) % Capacity, std::memory_order_release); // Hardware Barrier
        return true;
    }

    [[nodiscard]] bool Empty() const {
        return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed);
    }
};

#endif // SPSCQUEUE_Hf