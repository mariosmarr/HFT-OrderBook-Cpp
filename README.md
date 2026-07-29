Low-Latency C++ Matching Engine

A simple Limit Order Book and Order Matching Engine written in C++17 to study low-latency data structures and execution speed.

Architecture
To get O(1) cancellations and keep price-time priority (FIFO), the engine uses two main structures:

- std::unordered_map<int, std::unique_ptr<Order>>
  Master registry for O(1) order lookup and cancellation by ID using RAII.

- std::map<double, std::vector<Order*>>
  Price level queue that keeps prices sorted and stores raw pointers to preserve FIFO execution order.

Features
- Limit Orders (BID / ASK) with partial fills and resting volume.
- Market Orders with multi-level liquidity sweeps.
- Fast O(1) cancellations.
- Trade execution history with VWAP calculation.
- Silent mode to disable console prints during benchmarks.

Benchmark Results & Silent Mode (10,000 Orders)

I added a high-resolution micro-benchmark (Test 9) using std::chrono and a "Silent Mode" flag to measure pure engine speed without console I/O bottlenecks.

- Verbose Mode (with std::cout): 6,587 ms | ~658 us per order | 1,518 orders/sec
- Silent Mode (no std::cout)  : 4,410 ms | ~441 us per order | 2,267 orders/sec

Takeaway: Turning off console prints gave a ~33% speed boost because OS I/O calls are slow. Current latency is now mainly bottlenecked by dynamic heap allocations (std::make_unique).
