add_executable(ttcp_client ttcp_client.cpp)
target_link_libraries(ttcp_client
PRIVATE example_common 
PRIVATE xynet)

add_executable(ttcp_server ttcp_server.cpp)
target_link_libraries(ttcp_server
PRIVATE example_common 
PRIVATE xynet)