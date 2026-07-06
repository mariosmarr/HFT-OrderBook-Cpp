

#ifndef UNTITLED6_SELLORDER_H
#define UNTITLED6_SELLORDER_H
#include "Order.h"

class SellOrder : public Order {
public:
    SellOrder( double pricee) : Order( pricee) {
    }
    void Print() const override {
        std::cout << "[SELL] Order ID: " << id << " | Price: $" << price << std::endl;
    }
};

#endif