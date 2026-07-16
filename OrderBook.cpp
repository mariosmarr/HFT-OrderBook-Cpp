#include "OrderBook.h"

// Constructor: Reserves buckets in the hash map to prevent rehashing during high-volume periods
OrderBook::OrderBook() {
    orderMap.reserve(100);
}

// 1. Cancel Order
void OrderBook::CancelOrder(int id) {
    // Step 1: Locate the order in the main registry (O(1) complexity)
    auto itOrder = orderMap.find(id);

    if (itOrder == orderMap.end()) {
        std::cout << "[OrderBook] Cannot cancel. Order ID: " << id << " not found." << std::endl;
        return;
    }

    // Step 2: Extract the order's limit price to locate it in the Price Map
    double price = itOrder->second->GetPrice();
    auto itPrice = PriceMap.find(price);

    if (itPrice != PriceMap.end()) {
        auto& orderList = itPrice->second;

        // Step 3: Remove the non-owning raw pointer from the price queue
        for (auto itVector = orderList.begin(); itVector != orderList.end(); ++itVector) {
            Order* currentOrder = *itVector;
            if (currentOrder && currentOrder->GetId() == id) {
                orderList.erase(itVector);
                break;
            }
        }

        // Step 4: Clean up empty price levels to conserve memory
        if (orderList.empty()) {
            PriceMap.erase(itPrice);
        }
    }

    // Step 5: Release the order from the main registry (triggers RAII-based memory deallocation)
    orderMap.erase(itOrder);
    std::cout << "[OrderBook] Successfully cancelled Order ID: " << id << std::endl;
}

// 2. Insert Order
void OrderBook::InsertOrder(std::unique_ptr<Order> newOrder) {
    int id = newOrder->GetId();
    double price = newOrder->GetPrice();

    // Transfer unique ownership to the registry
    orderMap[id] = std::move(newOrder);

    // Keep a non-owning raw pointer for matching and sorting in the limit queue
    Order* rawOrderPtr = orderMap[id].get();
    PriceMap[price].push_back(rawOrderPtr);

    std::cout << "[OrderBook] Successfully inserted Order ID: " << id << " at Price: $" << price << std::endl;
}

// 3. Get Total Volume at a Specific Price Level
int OrderBook::GetVolumeAtLevel(double price) {
    auto it = PriceMap.find(price);
    if (it == PriceMap.end()) {
        return 0;
    }
    int totalvolume = 0;
    // Iterate through the queue and sum up outstanding shares
    for (const auto& orderPtr : it->second) {
        if (orderPtr) {
            totalvolume += orderPtr->GetAttribute();
        }
    }
    return totalvolume;
}

// 4. Core Matching Engine (FIFO Priority Execution)
void OrderBook::MatchOrder(std::unique_ptr<Order> newOrder) {
    // Prevent malformed or invalid orders from entering the system
    if (!ValidateOrder(newOrder)) {
        return;
    }
    double price = newOrder->GetPrice();
    int quantity = newOrder->GetAttribute();

    // =========================================================================
    // SCENARIO A: Incoming order is a BID (BUY) -> Match against existing ASKS (SELL)
    // =========================================================================
    if (newOrder->isBuy()) {
        auto itMap = PriceMap.begin();
        while (itMap != PriceMap.end() && quantity > 0) {
            double currentPrice = itMap->first;
            auto& orderList = itMap->second;

            // Check if the selling limit price is competitive enough
            if (currentPrice <= price) {
                auto itVector = orderList.begin();

                while (itVector != orderList.end() && quantity > 0) {
                    Order* existingOrder = *itVector;

                    // Ensure target is a valid Sell Order with outstanding volume
                    if (existingOrder && !existingOrder->isBuy() && existingOrder->GetAttribute() > 0) {
                        int tradeAmount = std::min(quantity, existingOrder->GetAttribute());

                        quantity -= tradeAmount;
                        int updatedExistingQty = existingOrder->GetAttribute() - tradeAmount;
                        existingOrder->SetQuantity(updatedExistingQty);

                       std::cout << "[TRADE EXECUTED] " << tradeAmount << " shares matched at $"
                                  << currentPrice << " (Buyer ID: " << newOrder->GetId() << ")" << std::endl;

                        // Fully executed order cleanup
                        if (existingOrder->GetAttribute() == 0) {
                            itVector = orderList.erase(itVector);
                            continue;
                        }
                    }
                    ++itVector;
                }

                if (orderList.empty()) {
                    itMap = PriceMap.erase(itMap);
                    continue;
                }
            }
            ++itMap;
        }
    }
    // =========================================================================
    // SCENARIO B: Incoming order is an ASK (SELL) -> Match against existing BIDS (BUY)
    // =========================================================================
    else {
        auto itMap = PriceMap.begin();
        while (itMap != PriceMap.end() && quantity > 0) {
            double currentPrice = itMap->first;
            auto& orderList = itMap->second;

            // Check if the buying limit price is competitive enough
            if (currentPrice >= price) {
                auto itVector = orderList.begin();

                while (itVector != orderList.end() && quantity > 0) {
                    Order* existingOrder = *itVector;

                    // Ensure target is a valid Buy Order with outstanding volume
                    if (existingOrder && existingOrder->isBuy() && existingOrder->GetAttribute() > 0) {
                        int tradeAmount = std::min(quantity, existingOrder->GetAttribute());

                        quantity -= tradeAmount;
                        int updatedExistingQty = existingOrder->GetAttribute() - tradeAmount;
                        existingOrder->SetQuantity(updatedExistingQty);

                        std::cout << "[TRADE EXECUTED] " << tradeAmount << " shares matched at $"
                                  << currentPrice << " (Seller ID: " << newOrder->GetId() << ")" << std::endl;

                        // Fully executed order cleanup
                        if (existingOrder->GetAttribute() == 0) {
                            itVector = orderList.erase(itVector);
                            continue;
                        }
                    }
                    ++itVector;
                }

                if (orderList.empty()) {
                    itMap = PriceMap.erase(itMap);
                    continue;
                }
            }
            ++itMap;
        }
    }

    // =========================================================================
    // RESIDUAL PROCESSING: Add unfilled/resting volume to the Order Book
    // =========================================================================
    if (quantity > 0) {
        newOrder->SetQuantity(quantity);
        InsertOrder(std::move(newOrder));
    }
}

// 5. Render Book State
void OrderBook::PrintOrderBook() const {
    std::cout << "\n============= CURRENT ORDER BOOK =============" << std::endl;

    if (PriceMap.empty()) {
        std::cout << "[Empty Book] No active orders in the market." << std::endl;
        std::cout << "==============================================" << std::endl;
        return;
    }

    for (const auto& pair : PriceMap) {
        double price = pair.first;
        const auto& orderList = pair.second;

        int levelVolume = 0;
        std::string side = "UNKNOWN";

        for (const auto& orderPtr : orderList) {
            if (orderPtr) {
                levelVolume += orderPtr->GetAttribute();
                side = orderPtr->isBuy() ? "BID (BUY)" : "ASK (SELL)";
            }
        }

        std::cout << "[" << side << "] Price: $" << price << " | Total Volume: " << levelVolume << " shares" << std::endl;
    }
    std::cout << "==============================================\n" << std::endl;
}

// Defensive Validation: Prevents corrupted inputs from poisoning the engine
bool OrderBook::ValidateOrder(const std::unique_ptr<Order>& order) const {
    if (!order) {
        std::cout << "[WARNING] Rejected: Order is null!" << std::endl;
        return false;
    }
    if (order->GetPrice() <= 0.0) {
        std::cout << "[WARNING] Rejected Order ID " << order->GetId()
                  << ": Price must be greater than 0! (Received: $" << order->GetPrice() << ")" << std::endl;
        return false;
    }
    if (order->GetAttribute() <= 0) {
        std::cout << "[WARNING] Rejected Order ID " << order->GetId()
                  << ": Quantity must be greater than 0! (Received: " << order->GetAttribute() << ")" << std::endl;
        return false;
    }
    return true;
}
void OrderBook::ExecuteMarketOrder(bool isBuySide) {

    // Sanity check: can't trade if the book has zero orders
    if (PriceMap.empty()) {
        std::cout << "\n⚠️ [MARKET ORDER] Failed! The Order Book is empty." << std::endl;
        return;
    }

    double bestPrice = -1.0;
    bool found = false;

    // =========================================================================
    // CASE A: MARKET BUY
    // We want to buy at the cheapest available sell price (Best Ask)
    // =========================================================================
    if (isBuySide) {

        // Scan the unordered map to find the lowest price level with active sellers
        for (const auto& pair : PriceMap) {
            double price = pair.first;
            const auto& orderList = pair.second;

            if (!orderList.empty()) {
                bool hasAsks = false;

                // Ensure this price level actually has Sell orders (asks)
                for (auto* o  : orderList) {
                    if (o && !o->isBuy()) {
                        hasAsks = true;
                        break; // Found a seller, skip the rest of this list for speed
                    }
                }

                // Keep track of the lowest valid sell price we've seen so far
                if (hasAsks) {
                    if (!found || price < bestPrice) {
                        bestPrice = price;
                        found = true;
                    }
                }
            }
        }

        // If a valid price level was found, match the first order (FIFO)
        if (found) {
            std::cout << "\n⚡ [MARKET BUY] Executed immediately at best price: $" << bestPrice << std::endl;

            auto& orderList = PriceMap[bestPrice];

            for (auto it = orderList.begin(); it != orderList.end(); ++it) {
                Order* orderToMatch = *it;

                if (orderToMatch && !orderToMatch->isBuy()) {
                    int matchedId = orderToMatch->GetId();

                    // Erase raw pointer first to avoid dangling pointer issues
                    orderList.erase(it);

                    // Erase from registry to safely deallocate unique_ptr memory (RAII)
                    orderMap.erase(matchedId);
                    break;
                }
            }

            // Cleanup empty price levels to keep memory footprint low
            if (orderList.empty()) {
                PriceMap.erase(bestPrice);
            }
        } else {
            std::cout << "\n⚠️ [MARKET BUY] Failed! No sell orders (asks) available in the book." << std::endl;
        }
    }
    // =========================================================================
    // CASE B: MARKET SELL
    // We want to sell at the highest available buy price (Best Bid)
    // =========================================================================
    else {
        // Scan the unordered map to find the highest price level with active buyers
        for (const auto& pair : PriceMap) {
            double price = pair.first;
            const auto& orderList = pair.second;

            if (!orderList.empty()) {
                bool hasBids = false;

                // Ensure this price level actually has Buy orders (bids)
                for ( auto* o : orderList) {
                    if (o && o->isBuy()) {
                        hasBids = true;
                        break;
                    }
                }

                // Keep track of the highest valid buy price we've seen so far
                if (hasBids) {
                    if (!found || price > bestPrice) {
                        bestPrice = price;
                        found = true;
                    }
                }
            }
        }

        // If a valid price level was found, match the first order (FIFO)
        if (found) {
            std::cout << "\n⚡ [MARKET SELL] Executed immediately at best price: $" << bestPrice << std::endl;

            auto& orderList = PriceMap[bestPrice];

            for (auto it = orderList.begin(); it != orderList.end(); ++it) {
                Order* orderToMatch = *it;

                if (orderToMatch && orderToMatch->isBuy()) {
                    int matchedId = orderToMatch->GetId();

                    // Safety cleanup of raw pointers and unique_ptr memory
                    orderList.erase(it);
                    orderMap.erase(matchedId);
                    break;
                }
            }

            // Cleanup empty price level
            if (orderList.empty()) {
                PriceMap.erase(bestPrice);
            }
        } else {
            std::cout << "\n⚠️ [MARKET SELL] Failed! No buy orders (bids) available in the book." << std::endl;
        }
    }
}