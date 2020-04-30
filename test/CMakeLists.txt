find_package(doctest REQUIRED)

add_executable(xynet_playground playground.cpp)
target_link_libraries(xynet_playground PRIVATE xynet)
target_compile_features(xynet_playground PRIVATE cxx_std_20)
target_compile_options(xynet_playground PRIVATE "-fcoroutines")