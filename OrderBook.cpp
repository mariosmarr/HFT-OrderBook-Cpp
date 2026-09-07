#include "OrderBook.h"

// Pre-size lookup registry to avoid reallocations
OrderBook::OrderBook() {
    orderRegistry.assign(MAX_ORDERS, nullptr);
}

// Basic sanity check for order fields
bool OrderBook::ValidateOrder(const Order* order) const {
    if (!order) {
        std::cout << "Warning: Rejected null order." << std::endl;
        return false;
    }
    if (order->GetPrice() <= 0.0) {
        std::cout << "Warning: Rejected Order ID " << order->GetId()
                  << " - Price must be > 0 (Received: $" << order->GetPrice() << ")" << std::endl;
        return false;
    }
    if (order->GetAttribute() <= 0) {
        std::cout << "Warning: Rejected Order ID " << order->GetId()
                  << " - Quantity must be > 0 (Received: " << order->GetAttribute() << ")" << std::endl;
        return false;
    }
    return true;
}

// Insert resting order into price array and update active sweep bounds
void OrderBook::InsertOrder(Order* newOrder) {
    int id = newOrder->GetId();
    double price = newOrder->GetPrice();
    int targetIndex = PriceToIndex(price);

    // Track order ptr by ID for O(1) cancel lookup
    if (id >= 0 && id < MAX_ORDERS) {
        orderRegistry[id] = newOrder;
    }

    priceArray[targetIndex].push_back(newOrder);

    // Track populated index limits to narrow down linear scan range
    if (targetIndex < minActiveIndex) minActiveIndex = targetIndex;
    if (targetIndex > maxActiveIndex) maxActiveIndex = targetIndex;

    if (!silentMode) {
        std::cout << "Inserted Order ID " << id << " at Price: $" << price << std::endl;
    }
}

// O(1) order lookup and queue removal
void OrderBook::CancelOrder(int id) {
    if (id < 0 || id >= MAX_ORDERS || orderRegistry[id] == nullptr) {
        std::cout << "Cannot cancel: Order ID " << id << " not found." << std::endl;
        return;
    }

    Order* orderToCancel = orderRegistry[id];
    int index = PriceToIndex(orderToCancel->GetPrice());
    auto& orderList = priceArray[index];

    // Find and remove pointer from the target price bucket
    for (auto itVector = orderList.begin(); itVector != orderList.end(); ++itVector) {
        if (*itVector && (*itVector)->GetId() == id) {
            orderList.erase(itVector);
            break;
        }
    }

    orderRegistry[id] = nullptr;
    std::cout << "Cancelled Order ID " << id << "." << std::endl;
}

// Aggregate volume at a given tick
int OrderBook::GetVolumeAtLevel(double price) {
    int index = PriceToIndex(price);
    int totalvolume = 0;

    for (const auto& orderPtr : priceArray[index]) {
        if (orderPtr) totalvolume += orderPtr->GetAttribute();
    }
    return totalvolume;
}

// Matching engine hot path: linear sweep through active price buckets
void OrderBook::MatchOrder(Order* newOrder) {
    if (!ValidateOrder(newOrder)) return;

    double price = newOrder->GetPrice();
    int quantity = newOrder->GetAttribute();
    int targetIndex = PriceToIndex(price);

    // BUY: sweeps asks starting from lowest active price up to limit
    if (newOrder->isBuy()) [[likely]] {
        int startSearch = std::max(0, minActiveIndex);

        for (int i = startSearch; i <= targetIndex && quantity > 0; ++i) {
            auto& orderList = priceArray[i];
            if (orderList.empty()) [[unlikely]] continue;

            auto itVector = orderList.begin();
            while (itVector != orderList.end() && quantity > 0) {
                Order* existingOrder = *itVector;

                if (existingOrder && !existingOrder->isBuy() && existingOrder->GetAttribute() > 0) {
                    int tradeAmount = std::min(quantity, existingOrder->GetAttribute());

                    quantity -= tradeAmount;
                    existingOrder->SetQuantity(existingOrder->GetAttribute() - tradeAmount);

                    tradeLedger.push_back({
                        newOrder->GetId(),
                        existingOrder->GetId(),
                        BASE_PRICE + (i / 100.0),
                        tradeAmount
                    });

                    // Order fully filled: drop from registry and price bucket
                    if (existingOrder->GetAttribute() == 0) {
                        orderRegistry[existingOrder->GetId()] = nullptr;
                        itVector = orderList.erase(itVector);
                        continue;
                    }
                }
                ++itVector;
            }
        }
    }
    // SELL: sweeps bids starting from highest active price down to limit
    else {
        int startSearch = std::min(static_cast<int>(MAX_PRICE_LEVELS) - 1, maxActiveIndex);

        for (int i = startSearch; i >= targetIndex && quantity > 0; --i) {
            auto& orderList = priceArray[i];
            if (orderList.empty()) [[unlikely]] continue;

            auto itVector = orderList.begin();
            while (itVector != orderList.end() && quantity > 0) {
                Order* existingOrder = *itVector;

                if (existingOrder && existingOrder->isBuy() && existingOrder->GetAttribute() > 0) {
                    int tradeAmount = std::min(quantity, existingOrder->GetAttribute());

                    quantity -= tradeAmount;
                    existingOrder->SetQuantity(existingOrder->GetAttribute() - tradeAmount);

                    tradeLedger.push_back({
                        existingOrder->GetId(),
                        newOrder->GetId(),
                        BASE_PRICE + (i / 100.0),
                        tradeAmount
                    });

                    // Order fully filled: drop from registry and price bucket
                    if (existingOrder->GetAttribute() == 0) {
                        orderRegistry[existingOrder->GetId()] = nullptr;
                        itVector = orderList.erase(itVector);
                        continue;
                    }
                }
                ++itVector;
            }
        }
    }

    // Unfilled balance rests on the book as passive liquidity
    if (quantity > 0) {
        newOrder->SetQuantity(quantity);
        InsertOrder(newOrder);
    }
}

// Sweeps available liquidity across active levels regardless of price
void OrderBook::ExecuteMarketOrder(bool isBuySide, int requestedQty) {
    if (requestedQty <= 0) return;

    std::cout << "\nMarket Order Started: Requested Qty: " << requestedQty
              << (isBuySide ? " (BUY)" : " (SELL)") << std::endl;

    while (requestedQty > 0) {
        int bestIndex = -1;

        // Locate best opposing quote within active range
        if (isBuySide) {
            for (int i = std::max(0, minActiveIndex); i <= maxActiveIndex; ++i) {
                if (!priceArray[i].empty()) {
                    for (auto* o : priceArray[i]) {
                        if (o && !o->isBuy()) { bestIndex = i; break; }
                    }
                }
                if (bestIndex != -1) break;
            }
        } else {
            for (int i = std::min(static_cast<int>(MAX_PRICE_LEVELS) - 1, maxActiveIndex); i >= minActiveIndex; --i) {
                if (!priceArray[i].empty()) {
                    for (auto* o : priceArray[i]) {
                        if (o && o->isBuy()) { bestIndex = i; break; }
                    }
                }
                if (bestIndex != -1) break;
            }
        }

        if (bestIndex == -1) break;

        // Execute against book liquidity at the selected tick
        double bestPrice = BASE_PRICE + (bestIndex / 100.0);
        auto& orderList = priceArray[bestIndex];
        auto it = orderList.begin();

        while (it != orderList.end() && requestedQty > 0) {
            Order* orderToMatch = *it;

            if (orderToMatch && (isBuySide ? !orderToMatch->isBuy() : orderToMatch->isBuy())) {
                int tradeAmount = std::min(requestedQty, orderToMatch->GetAttribute());
                requestedQty -= tradeAmount;
                orderToMatch->SetQuantity(orderToMatch->GetAttribute() - tradeAmount);

                std::cout << "Trade: Matched " << tradeAmount << " shares at $" << bestPrice
                          << " with Order ID: " << orderToMatch->GetId() << std::endl;

                if (orderToMatch->GetAttribute() == 0) {
                    orderRegistry[orderToMatch->GetId()] = nullptr;
                    it = orderList.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    if (requestedQty > 0) {
        std::cout << "Market Order Partial: Liquidity deficit. "
                  << requestedQty << " shares left unfulfilled." << std::endl;
    } else {
        std::cout << "Market Order Fully Filled." << std::endl;
    }
}

// Debug display utilities
void OrderBook::PrintOrderBook() const {
    std::cout << "\n--- Current Order Book ---" << std::endl;

    bool isEmpty = true;
    for (int i = std::max(0, minActiveIndex); i <= maxActiveIndex; ++i) {
        if (priceArray[i].empty()) continue;

        int levelVolume = 0;
        std::string side = "UNKNOWN";

        for (const auto& orderPtr : priceArray[i]) {
            if (orderPtr) {
                levelVolume += orderPtr->GetAttribute();
                side = orderPtr->isBuy() ? "BID" : "ASK";
                isEmpty = false;
            }
        }

        if (levelVolume > 0) {
            std::cout << side << " | Price: $" << (BASE_PRICE + (i / 100.0))
                      << " | Volume: " << levelVolume << " shares" << std::endl;
        }
    }

    if (isEmpty) {
        std::cout << "Empty Book: No active orders." << std::endl;
    }
    std::cout << "--------------------------\n" << std::endl;
}

void OrderBook::PrintTradeHistory() const {
    std::cout << "\n--- Trade Execution History ---" << std::endl;
    if (tradeLedger.empty()) {
        std::cout << "No trades were made." << std::endl;
        std::cout << "-------------------------------" << std::endl;
        return;
    }

    double totalTurnover = 0.0;
    int totalVolume = 0;

    for (const auto& trade : tradeLedger) {
        double tradeValue = trade.price * trade.quantity;
        totalTurnover += tradeValue;
        totalVolume += trade.quantity;

        std::cout << "Match: Buyer #" << trade.buyerId << " <-> Seller #" << trade.sellerId
                  << " | " << trade.quantity << " shares @ $" << trade.price
                  << " (Total: $" << tradeValue << ")" << std::endl;
    }

    double vwap = (totalVolume > 0) ? (totalTurnover / totalVolume) : 0.0;
    std::cout << "\nSummary:" << std::endl;
    std::cout << "- Total shares traded: " << totalVolume << std::endl;
    std::cout << "- Total money spent: $" << totalTurnover << std::endl;
    std::cout << "- Average execution price (VWAP): $" << vwap << std::endl;
    std::cout << "--------------------------------\n" << std::endl;
}

void OrderBook::SetSilentMode(bool enable) {
    silentMode = enable;
}