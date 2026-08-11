Low-Latency C++ Matching Engine

A multi-threaded Limit Order Book and Order Matching Engine written in C++17 to study low-latency data structures, lock-free concurrency, and execution speed.

Architecture

To achieve ultra-low execution latency and maintain price-time priority (FIFO), the engine combines the following core components:

- std::unordered_map<int, std::unique_ptr<Order>>
  Master registry for O(1) order lookup and safe cancellation by ID using RAII.

- std::map<double, std::vector<Order*>> & Flat Direct Indexing
  Price level structure that keeps limit orders sorted while preserving FIFO execution order.

- Custom Order Memory Pool (OrderPool)
  Pre-allocates contiguous memory blocks during engine initialization to eliminate dynamic heap allocations (malloc-free) inside the execution hot-path.

- Lock-Free SPSC Queue (SPSCQueue)
  Single-Producer Single-Consumer ring buffer using atomic memory orders (acquire/release) and 64-byte L1 cache-line alignment (`alignas(64)`) to eliminate thread locks and prevent False Sharing.

Features

- Limit Orders (BID / ASK) with partial fills and resting volume.
- Market Orders with multi-level liquidity sweeps.
- Fast O(1) order cancellations.
- Multi-Threaded Architecture: Independent producer thread (network) and consumer thread (matching engine).
- Trade execution history with VWAP (Volume-Weighted Average Price) calculation.
- High-Performance Custom Order Pool for sub-microsecond memory allocations.
- Real-time latency tracking (P50, P90, P99) and silent mode for precise benchmarking.

Benchmark Progression (10,000 Orders)
1. 
2. Standard Mode (with std::cout)               : 6,587 ms | ~658.00 us per order |     1,518 orders/sec
2. Silent Mode (no std::cout)                   : 4,410 ms | ~441.00 us per order |     2,267 orders/sec
3. Memory Pool Mode (O(1) In-Memory)            :     5 ms |   ~0.51 us per order | 2,000,000 orders/sec
4. Flat Array Direct Indexing                   :     9 ms |   ~0.98 us per order | 1,017,604 orders/sec
5. Multi-Threaded Lock-Free SPSC Queue          :    10 ms |   ~1.03 us per order |   963,112 orders/sec

Tail Latency Profiling (Multi-Threaded SPSC Mode)

- P50 (Median Latency) :   800 ns (0.8 us)
- P90 Latency          :  1600 ns (1.6 us)
- P99 (Tail Latency)   :  2500 ns (2.5 us)
- Max Latency          : 153.3 us (OS context switch / core scheduling)

Key Takeaway:
Eliminating dynamic heap allocations, aligning atomic indices to 64-byte L1 cache lines (`alignas(64)`), and replacing mutex locks with atomic acquire/release queues reduced median execution latency down to sub-microsecond levels (800ns P50).