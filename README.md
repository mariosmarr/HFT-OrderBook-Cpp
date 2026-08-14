Low-Latency C++ Matching Engine and Binary UDP Gateway

A multi-threaded Limit Order Book and Order Matching Engine written in C++20 to study low-latency data structures, lock-free concurrency, execution speed, and end-to-end UDP network ingestion.

Architecture

To achieve ultra-low execution latency and maintain price-time priority (FIFO), the engine combines the following core components:

External Client and Network Ingestion (Core 0)
A Python client dispatches 13-byte binary UDP datagrams. A Winsock2 UDP socket listens on Port 8080. When a packet arrives, an order object is fetched from a pre-allocated Memory Pool and pushed onto a lock-free queue.

Master Registry and Direct Indexing

std::unordered_map: Master registry for O(1) order lookup and safe cancellation by ID using RAII.

Flat Direct Indexing: Price level structure that keeps limit orders sorted while preserving FIFO execution order.

Custom Order Memory Pool (OrderPool)
Pre-allocates contiguous memory blocks during engine initialization to eliminate dynamic heap allocations (malloc-free) inside the execution hot-path.

Lock-Free SPSC Queue (SPSCQueue)
Single-Producer Single-Consumer ring buffer using atomic memory orders (acquire/release) and 64-byte L1 cache-line alignment (alignas(64)) to eliminate thread locks and prevent False Sharing.

Hardware Sympathy and Thread Affinity
Producer and consumer threads are pinned to isolated hardware cores (SetThreadAffinityMask) to maximize L1/L2 cache residency and reduce OS context switches.

Features

Limit Orders (BID / ASK) with partial fills and resting volume.

Market Orders with multi-level liquidity sweeps.

Fast O(1) order cancellations.

Multi-Threaded Architecture: Independent producer thread (network) and consumer thread (matching engine).

Trade execution history with VWAP (Volume-Weighted Average Price) calculation.

High-Performance Custom Order Pool for sub-microsecond memory allocations.

Binary Network Interface: Incoming network orders are parsed directly from raw memory with zero serialization overhead (#pragma pack).

Real-time latency tracking (P50, P90, P99) and silent mode for precise benchmarking.

Benchmark Progression (10,000 Orders)

Standard Mode (with console output) : 6,587 ms | ~658.00 us per order | 1,518 orders/sec

Silent Mode (no console output) : 4,410 ms | ~441.00 us per order | 2,267 orders/sec

Memory Pool Mode (O(1) In-Memory) : 5 ms | ~0.51 us per order | 2,000,000 orders/sec

Flat Array Direct Indexing : 9 ms | ~0.98 us per order | 1,017,604 orders/sec

Multi-Threaded Lock-Free SPSC Queue : 10 ms | ~1.03 us per order | 963,112 orders/sec

End-to-End UDP Ingestion and Core Pinning : - | ~1.20 us per order | Sub-microsecond core path

Tail Latency Profiling (Live UDP Ingestion Mode)

P50 (Median Latency) : 1200 ns (1.2 us)

P90 Latency : 1900 ns (1.9 us)

P99 (Tail Latency) : 3100 ns (3.1 us)

Max Latency : 146.3 us (OS network stack interrupt / core scheduling)

Key Takeaway:
Eliminating dynamic heap allocations, aligning atomic indices to 64-byte L1 cache lines (alignas(64)), replacing mutex locks with atomic acquire/release queues, and pinning threads to specific CPU cores reduced median matching latency from 658 microseconds down to 1.2 microseconds.