add_library(transport packet.cpp)

target_include_directories(transport PUBLIC ${PROJECT_SOURCE_DIR}/include)

add_executable(server server.cpp)
target_link_libraries(server PRIVATE transport)

add_executable(client client.cpp)
target_link_libraries(client PRIVATE transport)