add_executable(chat_server chat_server.cpp)
target_link_libraries(chat_server
PRIVATE example_common 
PRIVATE xynet)