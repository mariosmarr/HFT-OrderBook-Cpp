
#ifndef UNTITLED6_BUYORDER_H
#define UNTITLED6_BUYORDER_H
#include "Order.h"

class BuyOrder : public Order {
public:
    BuyOrder( double pricee) : Order( pricee) {
    }
    void Print() const override {
        std::cout << "[BUY] Order ID: " << id << " | Price: $" << price << std::endl;
    }
};

#endif