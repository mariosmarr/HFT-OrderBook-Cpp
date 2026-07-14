cmake_minimum_required(VERSION 4.0)
project(untitled6)

set(CMAKE_CXX_STANDARD 20)

add_executable(untitled6 main.cpp

        Order.h
        BuyOrder.h
        SellOrder.h
        Order.cpp
        OrderBook.h
        OrderBook.cpp)
