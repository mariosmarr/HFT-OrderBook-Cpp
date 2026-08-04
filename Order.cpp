#include "Order.h"


int Order::orderCounter = 0;

// Constructor// ΑΛΛΑΓΗ ΜΟΝΟ ΣΕ ΑΥΤΗ ΤΗ ΓΡΑΜΜΗ:
Order::Order(double pricee, int Attri, bool isBuy) : price(pricee), Attribute(Attri), isBuySide(isBuy) {
    id = ++orderCounter;
}
// Destructor
Order::~Order() {}

// Virtual/Fallback print method
void Order::Print() const {
    std::cout << "GENERIC ORDER" << std::endl;
}