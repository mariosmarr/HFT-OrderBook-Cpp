#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include "BuyOrder.h"
#include "SellOrder.h"
#include "OrderBook.h"
#include <chrono>
#include "OrderPool.h"
#include <algorithm>
#include <numeric>


void PrintLatencyStats(std::vector<uint64_t>& latencies) {
    if (latencies.empty()) return;

    // Ταξινόμηση για υπολογισμό percentiles
    std::sort(latencies.begin(), latencies.end());

    size_t count = latencies.size();
    uint64_t p50 = latencies[count * 0.50];
    uint64_t p90 = latencies[count * 0.90];
    uint64_t p99 = latencies[count * 0.99];
    uint64_t maxLat = latencies.back();

    std::cout << "\n📊 [LATENCY TAIL ANALYSIS]" << std::endl;
    std::cout << "  - P50 (Median) : " << p50 << " ns (" << (p50 / 1000.0) << " us)" << std::endl;
    std::cout << "  - P90          : " << p90 << " ns (" << (p90 / 1000.0) << " us)" << std::endl;
    std::cout << "  - P99 (Tail)   : " << p99 << " ns (" << (p99 / 1000.0) << " us)" << std::endl;
    std::cout << "  - Max Latency  : " << maxLat << " ns (" << (maxLat / 1000.0) << " us)" << std::endl;
}
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
    book.CancelOrder(3);

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

   // -----------------------------------------------------------------
    // TEST 6: Immediate Market Order Execution
    // -----------------------------------------------------------------
    std::cout << "\n[TEST 6] Testing Aggressive Market Orders..." << std::endl;

    // Add resting limit orders to create a clear spread
    std::cout << "Populating some resting orders to test market matching..." << std::endl;
    auto restingBid = std::make_unique<BuyOrder>(90.0, 10);   // Buyer waiting at $90
    auto restingAsk = std::make_unique<SellOrder>(120.0, 15); // Seller waiting at $120
    book.MatchOrder(std::move(restingBid));
    book.MatchOrder(std::move(restingAsk));

    std::cout << "--- Book state before market orders hit ---" << std::endl;
    book.PrintOrderBook();

    // FIX: Updated to match the new V2 signature (Direction, Quantity)
    std::cout << "Firing Market Buy for 15 shares..." << std::endl;
    book.ExecuteMarketOrder(true, 15);

    std::cout << "Firing Market Sell for 10 shares..." << std::endl;
    book.ExecuteMarketOrder(false, 10);

    std::cout << "--- Book state after TEST 6 ---" << std::endl;
    book.PrintOrderBook();

    // -----------------------------------------------------------------
    // TEST 7: Industrial-Grade V2 Market Order Sweep & Partial Fills
    // -----------------------------------------------------------------
    std::cout << "\n[TEST 7] Testing V2 Market Order Deep Sweep & Partial Fills..." << std::endl;

    // Injecting deep Ask liquidity (Sellers) across 3 distinct price levels
    std::cout << "Populating deep Ask liquidity..." << std::endl;
    book.MatchOrder(std::make_unique<SellOrder>(200.0, 10)); // Seller A: 10 shares @ $200
    book.MatchOrder(std::make_unique<SellOrder>(201.0, 15)); // Seller B: 15 shares @ $201
    book.MatchOrder(std::make_unique<SellOrder>(202.0, 20)); // Seller C: 20 shares @ $202

    std::cout << "--- Book state before deep market sweep ---" << std::endl;
    book.PrintOrderBook();

    // Fire a massive Market Buy of 35 shares.
    // EXPECTED BEHAVIOR:
    // - Consumes 10 shares @ $200 (Level fully cleared)
    // - Consumes 15 shares @ $201 (Level fully cleared)
    // - Consumes 10 shares @ $202 (Partial fill, leaving 10 shares resting at $202)
    std::cout << "Firing heavy Market Buy for 35 shares..." << std::endl;
    book.ExecuteMarketOrder(true, 35);

    std::cout << "\n--- Final Book State after V2 Deep Sweep ---" << std::endl;
    book.PrintOrderBook();
    // -----------------------------------------------------------------
    // TEST 8: Trade Ledger & History Summary
    // -----------------------------------------------------------------
    std::cout << "\n[TEST 8] Printing Trade History & Analytics..." << std::endl;

    // Call the execution journal to display all matched trades
    book.PrintTradeHistory();


    // TEST 9: High-Throughput & Latency Micro-Benchmark (10,000 Orders)
    std::cout << "\n--- TEST 9: Memory Pool Micro-Benchmark (10,000 Orders) ---" << std::endl;

    OrderBook benchBook;
    benchBook.SetSilentMode(true); // Mute cout to avoid OS I/O bottlenecks

    const int TOTAL_ORDERS = 10000;
    OrderPool pool(TOTAL_ORDERS); // Pre-allocate memory on RAM to avoid malloc overhead

    // Start high-resolution timer for Test 9
    auto benchStart = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= TOTAL_ORDERS; ++i) {
        // Generate simple dummy market data using modulo
        double price = 100.0 + (i % 20);
        int qty = 10 + (i % 50);

        Order* fastOrder = nullptr;


        if (i % 2 == 0) {
            fastOrder = pool.AllocateBuy(price, qty);
        } else {
            fastOrder = pool.AllocateSell(price, qty);
        }
        benchBook.MatchPooledOrder(fastOrder);
    }

    // Stop timer for Test 9
    auto benchEnd = std::chrono::high_resolution_clock::now();

    // Calculate total execution time and throughput metrics
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(benchEnd - benchStart).count();
    auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(benchEnd - benchStart).count();

    double avgLatencyUs = (double)durationUs / TOTAL_ORDERS;
    double ordersPerSec = (TOTAL_ORDERS / (double)durationMs) * 1000.0;

    // Print benchmark summary
    benchBook.SetSilentMode(false); // Unmute to print final results
    std::cout << "----------------------------------------------------" << std::endl;
    std::cout << " Pooled Benchmark Finished!" << std::endl;
    std::cout << " Total Time      : " << durationMs << " ms (" << durationUs << " us)" << std::endl;
    std::cout << " Avg Latency/Order: " << avgLatencyUs << " us" << std::endl;
    std::cout << " Engine Throughput: " << (int)ordersPerSec << " orders/sec" << std::endl;

// TEST 10: Flat Array Direct Indexing Benchmark (10,000 Orders)
    std::cout << "\n--- TEST 10: Flat Array Direct Indexing Benchmark (10,000 Orders) ---" << std::endl;

    OrderBook flatBook;
    flatBook.SetSilentMode(true);

    OrderPool flatPool(20000);

    // ⚡ 1. ΠΙΝΑΚΑΣ ΓΙΑ TAIL LATENCY MEASUREMENTS
    std::vector<uint64_t> latencies;
    latencies.reserve(TOTAL_ORDERS);

    auto flatStart = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= TOTAL_ORDERS; ++i) {
        double price = 100.0 + (i % 8);
        int qty = 10 + (i % 50);

        Order* fastOrder = nullptr;

        if (i % 2 == 0) {
            fastOrder = flatPool.AllocateBuy(price, qty);
        } else {
            fastOrder = flatPool.AllocateSell(price, qty);
        }

        // ⚡ 2. ΜΕΤΡΗΣΗ ΚΑΘΕ ΕΝΤΟΛΗΣ ΣΕ NANOSECONDS
        auto opStart = std::chrono::high_resolution_clock::now();

        flatBook.MatchFlatPooledOrder(fastOrder);

        auto opEnd = std::chrono::high_resolution_clock::now();
        uint64_t opDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(opEnd - opStart).count();
        latencies.push_back(opDuration);
    }

    auto flatEnd = std::chrono::high_resolution_clock::now();

    auto flatDurationUs = std::chrono::duration_cast<std::chrono::microseconds>(flatEnd - flatStart).count();
    double flatAvgLatencyUs = (double)flatDurationUs / TOTAL_ORDERS;
    double flatOrdersPerSec = (flatDurationUs > 0) ? ((double)TOTAL_ORDERS / flatDurationUs) * 1000000.0 : 0.0;

    std::cout << "----------------------------------------------------" << std::endl;
    std::cout << " Flat Array Benchmark Finished!" << std::endl;
    std::cout << " Total Time      : " << flatDurationUs << " us" << std::endl;
    std::cout << " Avg Latency/Order: " << flatAvgLatencyUs << " us" << std::endl;
    std::cout << " Engine Throughput: " << (long long)flatOrdersPerSec << " orders/sec" << std::endl;

    // ⚡ 3. ΕΚΤΥΠΩΣΗ P50, P90, P99 TAIL LATENCY
    PrintLatencyStats(latencies);

    return 0;
}