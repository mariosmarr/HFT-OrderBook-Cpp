#ifndef ORDER_H
#define ORDER_H

#include <iostream>

class Order {
protected:
    int id;
    double price;
    int Attribute;
    bool isBuySide;

    static int orderCounter;

public:
    // Constructors & Destructor
Order(double pricee, int Attri, bool isBuy = true);
    virtual ~Order();

    //  Hot-Path Inline Getters/Setters
    [[nodiscard]] inline int GetId() const { return id; }
    [[nodiscard]] inline double GetPrice() const { return price; }
    [[nodiscard]] inline int GetAttribute() const { return Attribute; }
    [[nodiscard]] inline bool isBuy() const { return isBuySide; }

    inline void SetPrice(double newprice) { price = newprice; }
    inline void SetQuantity(int newQuantity) { Attribute = newQuantity; }

    virtual void Print() const;
};

#endif // ORDER_H