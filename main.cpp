#include <iostream>
#include <memory>
#include <vector>

#include <unordered_map>
#include "BuyOrder.h"
#include "SellOrder.h"
#include "OrderBook.h"

void ProcessOrder(const Order& currentOrder) {
    std::cout << "--> Sending to Exchange: ";
    currentOrder.Print();
}
int main() {
  OrderBook orderBook;
    orderBook.AddOrderToLevel(std::make_unique<BuyOrder>(151.00));
    orderBook.AddOrderToLevel(std::make_unique<SellOrder>(151.00));
    orderBook.AddOrderToLevel(std::make_unique<SellOrder>(152.00));

    std::cout << "------------------------------------------" << std::endl;
    return 0;

}