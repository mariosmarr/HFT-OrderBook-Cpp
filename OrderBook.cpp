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
                        // Record execution in ledger
                        tradeLedger.push_back({
                            newOrder->isBuy() ? newOrder->GetId() : existingOrder->GetId(),
                            newOrder->isBuy() ? existingOrder->GetId() : newOrder->GetId(),
                            currentPrice,
                            tradeAmount
                        });

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
//record execution in ledger
                        tradeLedger.push_back({
                             newOrder->isBuy() ? newOrder->GetId() : existingOrder->GetId(),
                             newOrder->isBuy() ? existingOrder->GetId() : newOrder->GetId(),
                             currentPrice,
                             tradeAmount
                         });

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
void OrderBook::ExecuteMarketOrder(bool isBuySide, int requestedQty) {

    // Validation: Block execution if book is empty or requested volume is zero/negative
    if (PriceMap.empty() || requestedQty <= 0) {
        std::cout << "\n⚠️ [MARKET ORDER] Rejected. Invalid quantity or empty book." << std::endl;
        return;
    }

    std::cout << "\n⚡ [MARKET ORDER START] Requested Qty: " << requestedQty
              << (isBuySide ? " (BUY)" : " (SELL)") << std::endl;

    // CORE SWEEP ENGINE: Loop while there is remaining quantity AND liquidity in the book
    while (requestedQty > 0 && !PriceMap.empty()) {

        double bestPrice = -1.0;
        bool found = false;

        // =========================================================================
        // STEP 1: PRICE DISCOVERY (Locate Best Bid / Best Ask)
        // =========================================================================
        for (const auto& pair : PriceMap) {
            double price = pair.first;
            const auto& orderList = pair.second;

            if (!orderList.empty()) {
                bool hasOppositeOrders = false;

                // Check if this specific price level contains the counter-party orders we need
                for (auto* o : orderList) {
                    if (o && (isBuySide ? !o->isBuy() : o->isBuy())) {
                        hasOppositeOrders = true;
                        break; // Counter-party found, skip the rest of this list for speed
                    }
                }

                // Market BUY looks for Lowest Ask | Market SELL looks for Highest Bid
                if (hasOppositeOrders) {
                    if (!found || (isBuySide ? price < bestPrice : price > bestPrice)) {
                        bestPrice = price;
                        found = true;
                    }
                }
            }
        }

        // Break the sweep engine if no more matching counter-party liquidity exists
        if (!found) {
            break;
        }

        // =========================================================================
        // STEP 2: ORDER MATCHING & FIFO EXECUTION AT BEST PRICE LEVEL
        // =========================================================================
        auto& orderList = PriceMap[bestPrice];
        auto it = orderList.begin();

        // Process orders sequentially at the current best price level
        while (it != orderList.end() && requestedQty > 0) {
            Order* orderToMatch = *it;

            if (orderToMatch && (isBuySide ? !orderToMatch->isBuy() : orderToMatch->isBuy())) {

                // Calculate fill volume (minimum of what we need vs what the resting order has)
                int tradeAmount = std::min(requestedQty, orderToMatch->GetAttribute());

                // Deduct the traded volume from the incoming market order
                requestedQty -= tradeAmount;

                // Update the resting order's remaining volume
                int remainingOrderQty = orderToMatch->GetAttribute() - tradeAmount;
                orderToMatch->SetQuantity(remainingOrderQty);

                std::cout << "   [TRADE] Matched " << tradeAmount << " shares at $" << bestPrice
                          << " with Order ID: " << orderToMatch->GetId() << std::endl;

                // SCENARIO A: Full Fill -> The resting order is completely filled
                if (remainingOrderQty == 0) {
                    int matchedId = orderToMatch->GetId();

                    // Remove the non-owning raw pointer from the Price Map queue
                    it = orderList.erase(it);

                    // Remove from registry to trigger safe unique_ptr memory deallocation (RAII)
                    orderMap.erase(matchedId);
                    continue;
                }

                // SCENARIO B: Partial Fill
                // The resting order has surplus volume. It stays in the book with updated qty.
                // The incoming market order is now fully filled (requestedQty == 0), breaking the loop.
            }
            ++it;
        }

        // Garbage collection: Clean up empty price levels to minimize memory footprint
        if (orderList.empty()) {
            PriceMap.erase(bestPrice);
        }
    }

    // =========================================================================
    // STEP 3: FINAL EXECUTION SUMMARY
    // =========================================================================
    if (requestedQty > 0) {
        std::cout << "⚠️ [MARKET ORDER PARTIAL] Liquidity deficit! "
                  << requestedQty << " shares left unfulfilled." << std::endl;
    } else {
        std::cout << " [MARKET ORDER FULLY FILLED] Execution completed successfully." << std::endl;
    }
}
// 6. Print Execution Journal
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

        std::cout << "Match: Buyer #" << trade.buyerId
                  << " <-> Seller #" << trade.sellerId
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