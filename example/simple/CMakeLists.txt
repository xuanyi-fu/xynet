add_executable(simple_discard discard.cpp)
target_link_libraries(simple_discard 
PRIVATE example_common 
PRIVATE xynet)

add_executable(simple_daytime daytime.cpp)
target_link_libraries(simple_daytime 
PRIVATE example_common 
PRIVATE xynet)

add_executable(simple_echo echo.cpp)
target_link_libraries(simple_echo
PRIVATE example_common 
PRIVATE xynet)

add_executable(simple_echo_revised echo_revised.cpp)
target_link_libraries(simple_echo_revised
PRIVATE example_common 
PRIVATE xynet)
target_compile_options(simple_echo_revised PRIVATE "-O3")

add_executable(simple_chargen chargen.cpp)
target_link_libraries(simple_chargen
PRIVATE example_common 
PRIVATE xynet)

add_executable(simple_time time.cpp)
target_link_libraries(simple_time
PRIVATE example_common 
PRIVATE xynet)