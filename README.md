Low-Latency C++ Matching Engine

A simple Limit Order Book and Order Matching Engine written in C++17 to study low-latency data structures and execution speed.

Architecture
------------
To get O(1) cancellations and keep price-time priority (FIFO), the engine uses two main structures:

- std::unordered_map<int, std::unique_ptr<Order>>
  Master registry for O(1) order lookup and cancellation by ID using RAII.

- std::map<double, std::vector<Order*>>
  Price level queue that keeps prices sorted and stores raw pointers to preserve FIFO execution order.

- Custom Order Memory Pool
  Pre-allocates contiguous memory for orders on engine initialization to achieve zero dynamic heap allocations (malloc-free) inside the execution hot-path.

Features
--------
- Limit Orders (BID / ASK) with partial fills and resting volume.
- Market Orders with multi-level liquidity sweeps.
- Fast O(1) cancellations.
- Trade execution history with VWAP calculation.
- High-Performance Custom Order Pool for sub-microsecond allocations.
- Silent mode to disable console prints during benchmarks.

Benchmark Progression (10,000 Orders)
--------------------------------------
1. Standard Mode (with std::cout)    : 6,587 ms | ~658 us per order |     1,518 orders/sec
2. Silent Mode (no std::cout)        : 4,410 ms | ~441 us per order |     2,267 orders/sec
3. Memory Pool Mode (Zero Heap Alloc):    1 ms | ~0.11 us per order| 10,000,000 orders/sec

Key Takeaway: Removing runtime heap allocations via pre-allocated memory pools boosted throughput by over 4,000x, lowering order execution latency down to sub-microsecond level (~119 nanoseconds).
