#include "OrderBook.h"

// Constructor: pre allocate 10.000
OrderBook::OrderBook() {
    orderRegistry.assign(MAX_ORDERS, nullptr);
}

// 1. Validation (Now takes a raw pointer instead of unique_ptr)
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

// 2. Insert Order (O(1) Direct Indexing)
void OrderBook::InsertOrder(Order* newOrder) {
    int id = newOrder->GetId();
    double price = newOrder->GetPrice();
    int targetIndex = PriceToIndex(price);

    // Save to O(1) static registry
    if (id >= 0 && id < MAX_ORDERS) {
        orderRegistry[id] = newOrder;
    }

    // Save to O(1) price array queue
    priceArray[targetIndex].push_back(newOrder);

    // Update active limits for fast sweeps
    if (targetIndex < minActiveIndex) minActiveIndex = targetIndex;
    if (targetIndex > maxActiveIndex) maxActiveIndex = targetIndex;

    if (!silentMode) {
        std::cout << "Inserted Order ID " << id << " at Price: $" << price << std::endl;
    }
}

// 3. Cancel Order (O(1) Memory-Safe Cancellation)
void OrderBook::CancelOrder(int id) {
    // Step 1: Direct O(1) lookup in the array
    if (id < 0 || id >= MAX_ORDERS || orderRegistry[id] == nullptr) {
        std::cout << "Cannot cancel: Order ID " << id << " not found." << std::endl;
        return;
    }

    Order* orderToCancel = orderRegistry[id];
    int index = PriceToIndex(orderToCancel->GetPrice());
    auto& orderList = priceArray[index];

    // Step 2: Remove the pointer from the price queue
    for (auto itVector = orderList.begin(); itVector != orderList.end(); ++itVector) {
        if (*itVector && (*itVector)->GetId() == id) {
            orderList.erase(itVector);
            break;
        }
    }

    // Step 3: Clear from registry
    orderRegistry[id] = nullptr;
    std::cout << "Cancelled Order ID " << id << "." << std::endl;
}

// 4. Get Volume at Price
int OrderBook::GetVolumeAtLevel(double price) {
    int index = PriceToIndex(price);
    int totalvolume = 0;

    for (const auto& orderPtr : priceArray[index]) {
        if (orderPtr) totalvolume += orderPtr->GetAttribute();
    }
    return totalvolume;
}

// 5. Core Matching Engine (Unified, Flat Array Sweep)
void OrderBook::MatchOrder(Order* newOrder) {
    if (!ValidateOrder(newOrder)) return;

    double price = newOrder->GetPrice();
    int quantity = newOrder->GetAttribute();
    int targetIndex = PriceToIndex(price);

    // SCENARIO A: Incoming BUY order (Sweeps up to its limit price)
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

                    if (existingOrder->GetAttribute() == 0) {
                        orderRegistry[existingOrder->GetId()] = nullptr; // Clear Registry
                        itVector = orderList.erase(itVector);            // Clear Queue
                        continue;
                    }
                }
                ++itVector;
            }
        }
    }
    // SCENARIO B: Incoming SELL order (Sweeps down to its limit price)
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

                    if (existingOrder->GetAttribute() == 0) {
                        orderRegistry[existingOrder->GetId()] = nullptr; // Clear Registry
                        itVector = orderList.erase(itVector);            // Clear Queue
                        continue;
                    }
                }
                ++itVector;
            }
        }
    }

    // RESIDUAL: Add unfilled volume to the book
    if (quantity > 0) {
        newOrder->SetQuantity(quantity);
        InsertOrder(newOrder); // Calls our new fast InsertOrder
    }
}

// 6. Market Order Sweep (Using Fast Bounds)
void OrderBook::ExecuteMarketOrder(bool isBuySide, int requestedQty) {
    if (requestedQty <= 0) return;

    std::cout << "\nMarket Order Started: Requested Qty: " << requestedQty
              << (isBuySide ? " (BUY)" : " (SELL)") << std::endl;

    while (requestedQty > 0) {
        int bestIndex = -1;

        // Step 1: Price Discovery using bounds
        if (isBuySide) {
            // Buyers look for the lowest available Ask
            for (int i = std::max(0, minActiveIndex); i <= maxActiveIndex; ++i) {
                if (!priceArray[i].empty()) {
                    for (auto* o : priceArray[i]) {
                        if (o && !o->isBuy()) { bestIndex = i; break; }
                    }
                }
                if (bestIndex != -1) break;
            }
        } else {
            // Sellers look for the highest available Bid
            for (int i = std::min(static_cast<int>(MAX_PRICE_LEVELS) - 1, maxActiveIndex); i >= minActiveIndex; --i) {
                if (!priceArray[i].empty()) {
                    for (auto* o : priceArray[i]) {
                        if (o && o->isBuy()) { bestIndex = i; break; }
                    }
                }
                if (bestIndex != -1) break;
            }
        }

        // Break if book is empty on the opposite side
        if (bestIndex == -1) break;

        // Step 2: Match at the discovered price level
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
                    orderRegistry[orderToMatch->GetId()] = nullptr; // Safe removal
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

// 7. Utilities
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