add_executable(websocket_discard websocket_discard.cpp)
target_link_libraries(websocket_discard
PRIVATE example_common 
PRIVATE xynet)

add_executable(websocket_echo websocket_echo.cpp)
target_link_libraries(websocket_echo
PRIVATE example_common 
PRIVATE xynet)

add_executable(websocket_chat websocket_chat.cpp)
target_link_libraries(websocket_chat
PRIVATE example_common 
PRIVATE xynet)
add_compile_options(PRIVATE "-O0")