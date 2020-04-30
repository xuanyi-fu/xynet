find_package(doctest REQUIRED)

add_executable(xynet_playground playground.cpp)
target_link_libraries(xynet_playground PRIVATE xynet)
target_compile_features(xynet_playground PRIVATE cxx_std_20)
target_compile_options(xynet_playground PRIVATE "-fcoroutines")

add_executable(xynet_tests main.cpp coroutine_test.cpp socket_address_test.cpp)
target_link_libraries(xynet_tests PRIVATE xynet doctest::doctest)
target_compile_features(xynet_tests PRIVATE cxx_std_20)
target_compile_options(xynet_tests PRIVATE "-fcoroutines")

include(doctest)
doctest_discover_tests(xynet_tests)