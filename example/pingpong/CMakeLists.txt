add_executable(pingpong_client pingpong_client.cpp)
target_link_libraries(pingpong_client
PRIVATE example_common 
PRIVATE xynet)
target_compile_options(pingpong_client PRIVATE "-O3")

add_executable(pingpong_server pingpong_server.cpp)
target_link_libraries(pingpong_server
PRIVATE example_common 
PRIVATE xynet)
target_compile_options(pingpong_client PRIVATE "-O3")