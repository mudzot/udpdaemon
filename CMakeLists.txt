cmake_minimum_required(VERSION 2.8)

PROJECT (udpdaemon)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")

#Strip symbols to reduce size in release build
#set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Os")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Os")

file(GLOB udpdaemon_SOURCES
		UdpDaemon.h
        main.cpp
)
add_executable(udpdaemon ${udpdaemon_SOURCES})
