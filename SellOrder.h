// SellOrder.h
#ifndef UNTITLED6_SELLORDER_H
#define UNTITLED6_SELLORDER_H

#include "Order.h"

// Ask representation
class SellOrder : public Order {
public:
    SellOrder(double pricee, int Attri) : Order(pricee, Attri, false) {}

    void Print() const override {
        std::cout << "[SELL] Order ID: " << id << " | Price: $" << price << std::endl;
    }

    bool isBuy() const {
        return false;
    }
};

#endif // UNTITLED6_SELLORDER_H