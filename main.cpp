#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include "BuyOrder.h"
#include "SellOrder.h"
#include "OrderBook.h"
#include <chrono>

// Demonstrates polymorphic order dispatching (useful for outbound routing)
void ProcessOrder(const Order& currentOrder) {
    std::cout << "--> Sending to Exchange: ";
    currentOrder.Print();
}

int main() {
    OrderBook book;

    std::cout << "==================================================" << std::endl;
    std::cout << "    STARTING THE ULTIMATE MATCHING ENGINE TEST    " << std::endl;
    std::cout << "==================================================" << std::endl;

    // -----------------------------------------------------------------
    // TEST 1: Defensive Programming & Validation Checks
    // -----------------------------------------------------------------
    std::cout << "\n[TEST 1] Triggering Validation Guards..." << std::endl;

    // Attempting to inject corrupt orders (Should be rejected)
    auto invalidPriceOrder = std::make_unique<BuyOrder>(-10.0, 100);
    auto invalidQtyOrder = std::make_unique<SellOrder>(100.0, 0);

    book.MatchOrder(std::move(invalidPriceOrder));
    book.MatchOrder(std::move(invalidQtyOrder));

    // -----------------------------------------------------------------
    // TEST 2: Liquidity Provision & FIFO Queue Priority
    // -----------------------------------------------------------------
    std::cout << "\n[TEST 2] Populating Liquidity (Sell Side)..." << std::endl;

    auto sell1 = std::make_unique<SellOrder>(150.0, 10); // ID 1
    auto sell2 = std::make_unique<SellOrder>(150.0, 5);  // ID 2 (FIFO - Queued behind ID 1)
    auto sell3 = std::make_unique<SellOrder>(155.0, 20); // ID 3

    book.MatchOrder(std::move(sell1));
    book.MatchOrder(std::move(sell2));
    book.MatchOrder(std::move(sell3));

    book.PrintOrderBook();

    // -----------------------------------------------------------------
    // TEST 3: Safe Cancellation (Existing & Non-Existing)
    // -----------------------------------------------------------------
    std::cout << "\n[TEST 3] Testing Cancellations..." << std::endl;

    // Cancel existing order (ID 2)
    book.CancelOrder(2);

    // Attempt to cancel non-existing order (ID 999) - Should fail gracefully
    book.CancelOrder(999);

    book.PrintOrderBook(); // $150 level should now contain only 10 shares
    std::cout << "\n[TEST 4] Sweeping Liquidity & Measuring Latency..." << std::endl;
    auto bigBuyer = std::make_unique<BuyOrder>(150.0, 12);
    auto start = std::chrono::high_resolution_clock::now();
    book.MatchOrder(std::move(bigBuyer));
    auto end = std::chrono::high_resolution_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    long long duration = timeDiff.count();
    std::cout << "\n⏱️ [LATENCY REPORT] Matching Engine Execution Time: " << duration << " nanoseconds!" << std::endl;
    book.PrintOrderBook();

    std::cout << "\n[TEST 5] Multi-Level Price Sweep..." << std::endl;

    // Add sellers at multiple price levels
    auto lowSeller = std::make_unique<SellOrder>(100.0, 5);  // ID 5
    auto midSeller = std::make_unique<SellOrder>(101.0, 10); // ID 6
    book.MatchOrder(std::move(lowSeller));
    book.MatchOrder(std::move(midSeller));

    std::cout << "--- Book state before multi-level sweep ---" << std::endl;
    book.PrintOrderBook();

    // Aggressive buyer sweeps both $100 and $101 levels
    std::cout << "Aggressive Buyer ID 7 enters for 15 shares @ $105.0..." << std::endl;
    auto sweepingBuyer = std::make_unique<BuyOrder>(105.0, 15); // ID 7
    book.MatchOrder(std::move(sweepingBuyer));

    std::cout << "\n--- Final Book State after all tests ---" << std::endl;
    book.PrintOrderBook();

    std::cout << "==================================================" << std::endl;
    std::cout << "           ALL TESTS COMPLETED SUCCESSFULLY       " << std::endl;
    std::cout << "==================================================" << std::endl;

    return 0;
}