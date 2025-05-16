cmake_minimum_required(VERSION 3.10)
project(UDPFileTransfer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

set(SHARED_SRC
    src/packet.cpp    
)

add_executable(client src/client.cpp ${SHARED_SRC})

add_executable(server src/server.cpp ${SHARED_SRC})