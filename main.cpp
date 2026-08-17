#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <thread>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "BuyOrder.h"
#include "SellOrder.h"
#include "OrderBook.h"
#include "OrderPool.h"
#include "SPSCQueue.h"

#pragma comment(lib, "ws2_32.lib")

#pragma pack(push, 1)
struct NetworkOrder {
    uint8_t isBuy;
    double price;
    int32_t qty;
};
#pragma pack(pop)

void PrintLatencyStats(std::vector<uint64_t>& latencies) {
    if (latencies.empty()) return;

    std::sort(latencies.begin(), latencies.end());

    size_t count = latencies.size();
    // Προσθήκη static_cast για να φύγουν τα Narrowing conversion warnings
    uint64_t p50 = latencies[static_cast<size_t>(count * 0.50)];
    uint64_t p90 = latencies[static_cast<size_t>(count * 0.90)];
    uint64_t p99 = latencies[static_cast<size_t>(count * 0.99)];
    uint64_t maxLat = latencies.back();

    std::cout << "\n[LATENCY TAIL ANALYSIS]" << std::endl;
    std::cout << "  - P50 (Median) : " << p50 << " ns (" << (p50 / 1000.0) << " us)" << std::endl;
    std::cout << "  - P90          : " << p90 << " ns (" << (p90 / 1000.0) << " us)" << std::endl;
    std::cout << "  - P99 (Tail)   : " << p99 << " ns (" << (p99 / 1000.0) << " us)" << std::endl;
    std::cout << "  - Max Latency  : " << maxLat << " ns (" << (maxLat / 1000.0) << " us)" << std::endl;
}

int main() {
    OrderBook book;
    // Pre-allocate 100,000 orders to serve all our tests without a single 'new'
    OrderPool globalPool(100000);

    std::cout << "    STARTING THE ULTIMATE MATCHING ENGINE TEST    " << std::endl;

    // -----------------------------------------------------------------
    // TEST 1 & 2: Liquidity Provision
    // -----------------------------------------------------------------
    std::cout << "\n[TEST 1 & 2] Populating Liquidity (Sell Side)..." << std::endl;

    Order* sell1 = globalPool.AllocateSell(150.0, 10);
    Order* sell2 = globalPool.AllocateSell(150.0, 5);
    Order* sell3 = globalPool.AllocateSell(155.0, 20);

    book.MatchOrder(sell1);
    book.MatchOrder(sell2);
    book.MatchOrder(sell3);

    book.PrintOrderBook();

    // -----------------------------------------------------------------
    // TEST 3: Safe Cancellation
    // -----------------------------------------------------------------
    std::cout << "\n[TEST 3] Testing O(1) Cancellations..." << std::endl;
    book.CancelOrder(sell2->GetId()); // Cancel the second order
    book.PrintOrderBook();

    // -----------------------------------------------------------------
    // TEST 4 & 5: Liquidity Sweep
    // -----------------------------------------------------------------
    std::cout << "\n[TEST 4 & 5] Multi-Level Price Sweep..." << std::endl;

    Order* lowSeller = globalPool.AllocateSell(100.0, 5);
    Order* midSeller = globalPool.AllocateSell(101.0, 10);
    book.MatchOrder(lowSeller);
    book.MatchOrder(midSeller);

    Order* sweepingBuyer = globalPool.AllocateBuy(105.0, 15);
    book.MatchOrder(sweepingBuyer);

    book.PrintOrderBook();

    // -----------------------------------------------------------------
    // TEST 6: Aggressive Market Orders
    // -----------------------------------------------------------------
    std::cout << "\n[TEST 6] Testing Aggressive Market Orders..." << std::endl;

    book.ExecuteMarketOrder(true, 15);  // Buy 15 shares
    book.ExecuteMarketOrder(false, 10); // Sell 10 shares

    book.PrintOrderBook();
    book.PrintTradeHistory();

    // -----------------------------------------------------------------
    // TEST 7: Zero-Allocation Single-Thread Benchmark (10,000 Orders)
    // -----------------------------------------------------------------
    std::cout << "\n--- TEST 7: Zero-Allocation Single-Thread Benchmark (10,000 Orders) ---" << std::endl;

    OrderBook benchBook;
    benchBook.SetSilentMode(true);

    const int TOTAL_ORDERS = 10000;
    std::vector<uint64_t> latencies;
    latencies.reserve(TOTAL_ORDERS);

    auto benchStart = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= TOTAL_ORDERS; ++i) {
        double price = 100.0 + (i % 8);
        int qty = 10 + (i % 50);

        // Αντικατάσταση του ternary operator με καθαρό if/else
        Order* fastOrder = nullptr;
        if (i % 2 == 0) {
            fastOrder = globalPool.AllocateBuy(price, qty);
        } else {
            fastOrder = globalPool.AllocateSell(price, qty);
        }

        auto opStart = std::chrono::high_resolution_clock::now();
        benchBook.MatchOrder(fastOrder);
        auto opEnd = std::chrono::high_resolution_clock::now();

        latencies.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(opEnd - opStart).count());
    }

    auto benchEnd = std::chrono::high_resolution_clock::now();
    auto benchDurationUs = std::chrono::duration_cast<std::chrono::microseconds>(benchEnd - benchStart).count();

    double benchAvgLatencyUs = (double)benchDurationUs / TOTAL_ORDERS;
    double benchOrdersPerSec = (benchDurationUs > 0) ? ((double)TOTAL_ORDERS / benchDurationUs) * 1000000.0 : 0.0;

    std::cout << " Total Time      : " << benchDurationUs << " us" << std::endl;
    std::cout << " Avg Latency     : " << benchAvgLatencyUs << " us" << std::endl;
    std::cout << " Engine Throughput: " << (long long)benchOrdersPerSec << " orders/sec" << std::endl;

    PrintLatencyStats(latencies);

    // -----------------------------------------------------------------
    // TEST 8: Multi-Threaded UDP Lock-Free Benchmark (10,000 Orders)
    // -----------------------------------------------------------------
    std::cout << "\n--- TEST 8: End-to-End UDP SPSC Lock-Free Benchmark (10,000 Orders) ---" << std::endl;

    OrderBook threadedBook;
    threadedBook.SetSilentMode(true);

    SPSCQueue<Order*, 20000> spscQueue;
    std::vector<uint64_t> threadLatencies;
    threadLatencies.reserve(TOTAL_ORDERS);

    // CORE PINNING: CONSUMER THREAD
    HANDLE mainThread = GetCurrentThread();
    SetThreadAffinityMask(mainThread, (1ULL << 1));

    auto spscStart = std::chrono::high_resolution_clock::now();

    // PRODUCER THREAD (UDP Network Server)
    std::thread producer([&]() {
        SetThreadAffinityMask(GetCurrentThread(), (1ULL << 0));

        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(8080);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        bind(udpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
        std::cout << "\n[NETWORK] UDP Server listening on Port 8080..." << std::endl;

        NetworkOrder netOrder;
        for (int i = 1; i <= TOTAL_ORDERS; ++i) {
            int bytesReceived = recvfrom(udpSocket, (char*)&netOrder, sizeof(NetworkOrder), 0, nullptr, nullptr);

            if (bytesReceived == sizeof(NetworkOrder)) {

                // Αντικατάσταση του ternary operator με καθαρό if/else
                Order* fastOrder = nullptr;
                if (netOrder.isBuy == 1) {
                    fastOrder = globalPool.AllocateBuy(netOrder.price, netOrder.qty);
                } else {
                    fastOrder = globalPool.AllocateSell(netOrder.price, netOrder.qty);
                }

                while (!spscQueue.Push(fastOrder)) {
                    std::this_thread::yield();
                }
            }
        }
        closesocket(udpSocket);
        WSACleanup();
    });

    // CONSUMER THREAD (Matching Engine)
    int processedCount = 0;
    while (processedCount < TOTAL_ORDERS) {
        Order* incomingOrder = nullptr;

        if (spscQueue.Pop(incomingOrder)) {
            auto opStart = std::chrono::high_resolution_clock::now();
            threadedBook.MatchOrder(incomingOrder);
            auto opEnd = std::chrono::high_resolution_clock::now();

            threadLatencies.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(opEnd - opStart).count());
            processedCount++;
        } else {
            std::this_thread::yield();
        }
    }

    producer.join();

    auto spscEnd = std::chrono::high_resolution_clock::now();
    auto spscDurationUs = std::chrono::duration_cast<std::chrono::microseconds>(spscEnd - spscStart).count();

    double spscAvgLatencyUs = (double)spscDurationUs / TOTAL_ORDERS;
    double spscOrdersPerSec = (spscDurationUs > 0) ? ((double)TOTAL_ORDERS / spscDurationUs) * 1000000.0 : 0.0;

    std::cout << " Total Multi-Thread Time: " << spscDurationUs << " us" << std::endl;
    std::cout << " Avg Execution Latency  : " << spscAvgLatencyUs << " us" << std::endl;
    std::cout << " Real-Time Throughput   : " << static_cast<long long>(spscOrdersPerSec) << " orders/sec" << std::endl;

    PrintLatencyStats(threadLatencies);
    return 0;
}