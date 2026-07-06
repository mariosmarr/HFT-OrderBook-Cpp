
//
// Created by mario on 6/30/2026.
//

#ifndef UNTITLED6_ORDER_H
#define UNTITLED6_ORDER_H
#include <iostream>


class Order {
protected:
    int id;
    static int orderCounter;
    double price;
public:
    int GetId()const
    {
        return id;
    }
    void SetPrice(double newprice) {
        price = newprice;

    }
    double GetPrice() const { return price; }
    Order(double pricee) :  price(pricee) {
        id=++orderCounter;
    }

    virtual void Print() const {
        std::cout << "GENERIC ORDER" << std::endl;
    }

    virtual ~Order() {
        std::cout << "Order " << id << " destroyed" << std::endl;
    }
};



#endif