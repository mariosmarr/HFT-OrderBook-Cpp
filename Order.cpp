#include "Order.h"

// Initialize static member to track total orders created in the session
int Order::orderCounter = 0;

// Constructor: Auto-increments ID to guarantee uniqueness for each order
Order::Order(double pricee, int Attri) : price(pricee), Attribute(Attri) {
    id = ++orderCounter;
}

// Destructor: Used to track the lifecycle and memory release of orders
Order::~Order() {
    std::cout << "Order " << id << " destroyed" << std::endl;
}

// Getters & Setters Implementation
int Order::GetId() const {
    return id;
}

double Order::GetPrice() const {
    return price;
}

void Order::SetPrice(double newprice) {
    price = newprice;
}

int Order::GetAttribute() const {
    return Attribute;
}

void Order::SetQuantity(int newQuantity) {
    Attribute = newQuantity;
}

// Fallback print method
void Order::Print() const {
    std::cout << "GENERIC ORDER" << std::endl;
}