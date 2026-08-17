Ultra-Low Latency C++ Matching Engine and Binary UDP Gateway
A multi-threaded Limit Order Book (LOB) and Order Matching Engine written in C++20. This project serves as an advanced study in low-latency data structures, zero-allocation memory management, lock-free concurrency, and end-to-end UDP network ingestion.

Architecture
To achieve sub-microsecond execution latency and maintain strict price-time priority (FIFO), the engine replaces traditional node-based data structures with contiguous, cache-friendly alternatives, combining the following core components:

External Client and Network Ingestion
A Python client dispatches 13-byte binary UDP datagrams. A Winsock2 UDP socket listens on Port 8080. Incoming packets are parsed directly from raw memory with zero serialization overhead using #pragma pack(1). The network thread fetches a pre-allocated order object and pushes it onto a lock-free queue.

Zero-Allocation Order Book
All standard library associative containers (std::map, std::unordered_map) have been completely removed to prevent dynamic heap allocations (new/delete) and hashing overhead during the execution hot-path.

Master Registry: A static array of raw pointers acts as the central order registry. Order IDs map directly to array indices, guaranteeing true O(1) lookups and cancellations without hash collisions.

Flat Direct Indexing: Price levels are managed via a fixed-size, contiguous array (priceArray). This structure eliminates the need for Red-Black tree allocations and significantly improves spatial locality during deep market order sweeps.

Hardware Sympathy and Memory Management
Custom Order Pool: A memory pool pre-allocates contiguous memory blocks for all order objects during engine initialization.

Cache-Line Alignment: The price level array is strictly aligned to 64-byte L1 cache lines (alignas(64)). This ensures sequential memory reads are highly optimized by the CPU's hardware prefetcher.

Lock-Free SPSC Queue: A Single-Producer Single-Consumer ring buffer utilizes atomic memory orders (acquire/release) to pass data between threads without OS-level mutex locks, avoiding thread sleeping and false sharing.

Thread Affinity: The producer thread (network ingestion) and consumer thread (matching engine) are explicitly pinned to isolated hardware CPU cores to maximize L1/L2 cache residency and eliminate OS context switching.

Features
Limit Orders (BID / ASK) with partial fills and resting volume logic.

Market Orders capable of multi-level liquidity sweeps.

Deterministic O(1) order cancellations.

Trade execution ledger with Volume-Weighted Average Price (VWAP) calculation.

Real-time tail latency tracking (P50, P90, P99).

Performance and Tail Latency Profiling
The system has been benchmarked using 10,000 deterministic orders. The measurements focus on the raw matching engine throughput and the end-to-end latency via local UDP loopback.

Zero-Allocation Single-Thread Benchmark

Throughput: ~898,000 orders/sec

P50 (Median Latency): 800 ns (0.8 us)

P90 Latency: 1800 ns (1.8 us)

P99 Latency: 3000 ns (3.0 us)

End-to-End UDP Ingestion and Core Pinning

P50 (Median Latency): 900 ns (0.9 us)

P90 Latency: 1500 ns (1.5 us)

P99 Latency: 2700 ns (2.7 us)

Max Latency: ~186 us (Attributed to OS network stack interrupts and socket buffering)

Key Takeaway
The transition from dynamic associative containers to strictly aligned flat arrays, combined with a custom memory pool and lock-free thread synchronization, successfully reduced median matching latency from over 600 microseconds down to sub-microsecond levels (800-900 nanoseconds).