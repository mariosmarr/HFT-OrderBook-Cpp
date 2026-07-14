#ifndef UNTITLED6_ORDER_H
#define UNTITLED6_ORDER_H

#include <iostream>

class Order {
protected:
    int Attribute; // Represents quantity/shares of the order
    int id;
    static int orderCounter; // Shared across all instances for auto-increment IDs
    double price;

public:
    // Constructor & Destructor
    Order(double pricee, int Attri);
    virtual ~Order();

    // Getters & Setters (Encapsulation)
    int GetId() const;
    double GetPrice() const;
    void SetPrice(double newprice);
    int GetAttribute() const;
    void SetQuantity(int newQuantity);

    // Polymorphic Methods
    virtual void Print() const;
    virtual bool isBuy() = 0; // Pure Virtual - enforces Buy/Sell distinction
};

#endif // UNTITLED6_ORDER_H