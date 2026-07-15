# C++ HFT Matching Engine

This is my personal project where I built a low-latency Limit Order Book from scratch using C++.

My goal was to understand how modern financial exchanges (like NASDAQ or crypto platforms) match buyers and sellers at high speeds, and how to write clean, high-performance code.

## What I Built
- **Limit Order Book:** Supports Buy and Sell limits with standard Price-Time priority.
- **Dynamic Memory:** Implemented using smart pointers (`std::unique_ptr`) for safe resource management.
- **Data Structures:** Used `std::map` to sort price levels and `std::vector` to queue orders.

## Performance & Testing
I integrated `std::chrono` to measure the matching engine's execution time in nanoseconds. On my local machine, matching an aggressive order takes about:
- **~100 to 300 nanoseconds** (depending on CPU cache state).

## Why I Built This
I created this to learn the core principles of low-latency C++ programming, memory management, and how high-frequency trading (HFT) systems handle massive order flows.