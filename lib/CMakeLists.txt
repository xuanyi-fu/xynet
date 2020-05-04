find_package(g3logger CONFIG REQUIRED)
find_package(liburing REQUIRED)

list(APPEND LIBS g3logger)
list(APPEND LIBS liburing::liburing)

add_library(xynet io_service.cpp)
target_include_directories(xynet PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(xynet PRIVATE ${LIBS})
target_compile_features(xynet PUBLIC cxx_std_20)
target_compile_options(xynet PUBLIC "-fcoroutines" PUBLIC "-O3")