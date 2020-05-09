add_executable(file_transfer file_transfer.cpp)
target_link_libraries(file_transfer
PRIVATE example_common 
PRIVATE xynet)
