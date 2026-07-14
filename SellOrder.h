#ifndef UNTITLED6_SELLORDER_H
#define UNTITLED6_SELLORDER_H

#include "Order.h"

// Represents an Ask (Sell side of the Order Book)
class SellOrder : public Order {
public:
    SellOrder(double pricee, int Attri) : Order(pricee, Attri) {}

    // Overriding polymorphic methods
    void Print() const override {
        std::cout << "[SELL] Order ID: " << id << " | Price: $" << price << std::endl;
    }

    bool isBuy() override {
        return false;
    }
};

#endif // UNTITLED6_SELLORDER_H