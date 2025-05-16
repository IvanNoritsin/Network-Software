cmake_minimum_required(VERSION 3.10)
project(dhcp_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

set(SOURCES
    src/main.cpp
    src/dhcp_server.cpp
)

add_executable(dhcp_server ${SOURCES})
