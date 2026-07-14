#ifndef UNTITLED6_BUYORDER_H
#define UNTITLED6_BUYORDER_H

#include "Order.h"

// Represents a Bid (Buy side of the Order Book)
class BuyOrder : public Order {
public:
    BuyOrder(double pricee, int Attri) : Order(pricee, Attri) {}

    // Overriding polymorphic methods
    void Print() const override {
        std::cout << "[BUY] Order ID: " << id << " | Price: $" << price << std::endl;
    }

    bool isBuy() override {
        return true;
    }
};

#endif // UNTITLED6_BUYORDER_H