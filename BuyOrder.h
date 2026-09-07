// BuyOrder.h
#ifndef UNTITLED6_BUYORDER_H
#define UNTITLED6_BUYORDER_H

#include "Order.h"

// Bid representation
class BuyOrder : public Order {
public:
    BuyOrder(double pricee, int Attri) : Order(pricee, Attri, true) {}

    void Print() const override {
        std::cout << "[BUY] Order ID: " << id << " | Price: $" << price << std::endl;
    }

    bool isBuy() const {
        return true;
    }
};

#endif // UNTITLED6_BUYORDER_H