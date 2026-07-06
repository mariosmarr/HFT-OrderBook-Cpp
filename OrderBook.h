#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <unordered_map>
#include <memory>
#include <iostream>
#include "Order.h"






class OrderBook {
private:
    std::unordered_map<int, std::unique_ptr<Order>> orderMap;
std::unordered_map<double, std::vector<std::unique_ptr<Order>>> PriceMap;
public:
    OrderBook() {
        orderMap.reserve(100);
    }

    void AddOrder(std::unique_ptr<Order> newOrder) {
        int id = newOrder->GetId();
        orderMap[id] = std::move(newOrder);
        std::cout << "[OrderBook] Automatically added Order ID: " << id << std::endl;
    }
    void CancelOrder(int id) {
        auto it=orderMap.find(id);
        if (it!=orderMap.end()) {
            orderMap.erase(it);
        }else {
            std::cout << "[OrderBook] Order ID not found" << std::endl;
        }
    }
    void AddOrderToLevel(std::unique_ptr<Order> newOrder) {
        double levelPrice=newOrder->GetPrice();
        PriceMap[levelPrice].push_back(std::move(newOrder));
    }
};

#endif