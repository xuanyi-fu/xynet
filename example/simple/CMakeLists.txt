add_executable(simple_discard discard.cpp)
target_link_libraries(simple_discard 
PRIVATE example_common 
PRIVATE xynet)

add_executable(simple_daytime daytime.cpp)
target_link_libraries(simple_daytime 
PRIVATE example_common 
PRIVATE xynet)