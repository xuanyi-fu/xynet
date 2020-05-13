add_executable(websocket_discard websocket_discard.cpp)
target_link_libraries(websocket_discard
PRIVATE example_common 
PRIVATE xynet)
target_compile_options(websocket_discard PRIVATE "-O0" PRIVATE "-g")