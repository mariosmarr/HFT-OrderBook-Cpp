// Order.cpp
#include "Order.h"

int Order::orderCounter = 0;

Order::Order(double pricee, int Attri, bool isBuy)
    : price(pricee), Attribute(Attri), isBuySide(isBuy) {
    id = ++orderCounter;
}

Order::~Order() {}

void Order::Print() const {
    std::cout << "GENERIC ORDER" << std::endl;
}