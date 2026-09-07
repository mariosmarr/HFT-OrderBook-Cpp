HFT OrderBook in C++ (Low Latency & UDP)

This is a personal project I built to study low-latency systems, lock-free concurrency, and hardware-conscious C++20. It's a multi-threaded Limit Order Book (LOB) and matching engine that processes UDP packets and matches orders.

The main goal was to see how fast I could make it by completely removing dynamic memory allocations (no new/delete) and standard library trees (std::map) from the execution hot path.

How it works (Architecture)
The setup has two pinned threads communicating via a lock-free queue:

Ingestion Thread (Core 0): Listens on a Winsock2 UDP socket (port 8080). A Python client sends 13-byte packed binary datagrams. I use #pragma pack(1) so the C++ side casts the raw memory directly into a struct—zero serialization overhead. It grabs a pre-allocated order and pushes it to the queue.

Matching Engine (Core 1): Pops orders from the queue and executes them against the book.

Zero-Allocation & Cache Locality
To get the latency under 1 microsecond, I had to redesign the data structures:

No std::map: Price levels are managed by a fixed-size contiguous array (priceArray). A price maps directly to an index (e.g. price * 100). This avoids red-black tree pointer chasing and keeps cache locality tight during deep market order sweeps.

Order Registry: Order IDs map directly to a static array of raw pointers. Lookups and cancellations are strictly O(1) without hash collisions.

OrderPool: I wrote a custom memory pool that pre-allocates contiguous memory for all orders at startup.

False Sharing Prevention: The price array and the SPSC (Single-Producer Single-Consumer) queue indices are padded with alignas(64) to sit on different L1 cache lines.

Lock-Free: The SPSC ring buffer uses std::memory_order_acquire / release. No OS mutexes or thread sleeping.

Features

Limit Orders (BID/ASK) with partial fills.

Market Orders that sweep liquidity across multiple price levels.

O(1) deterministic order cancellations.

Trade ledger tracking Volume-Weighted Average Price (VWAP).

Real-time tail latency profiling (P50, P90, P99).

Benchmarks
Tested locally with 10,000 deterministic orders.

Single-Thread Engine Benchmark (No Network)
Raw matching engine throughput:

Throughput: ~1,004,200 orders/sec

P50 (Median): 800 ns (0.8 us)

P90: 1500 ns (1.5 us)

P99: 2200 ns (2.2 us)

Dual-Thread UDP Pipeline (End-to-End)

P50 (Median): 1000 ns (1.0 us)

P90: 1600 ns (1.6 us)

P99: 2500 ns (2.5 us)

Max Latency: ~248 us

Next Steps & Observations
Dropping std::map and new/delete dropped my median matching latency from over 600us down to 800ns. However, looking at the dual-thread benchmark, the OS network stack (kernel socket buffering, syscall interrupts) adds significant tail latency (the 248us spike).

My next goal is to research kernel-bypass networking and user-space I/O to completely bypass the OS network stack and see how much closer to the metal I can get.