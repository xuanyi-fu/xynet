cmake_minimum_required(VERSION 3.16.3)
project(xynet)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(liburing REQUIRED)
find_package(OpenSSL REQUIRED)

list(APPEND LIBS liburing::liburing)
list(APPEND LIBS OpenSSL::Crypto)

add_library(xynet INTERFACE)
target_include_directories(xynet INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(xynet INTERFACE ${LIBS})
target_compile_features(xynet INTERFACE cxx_std_20)
target_compile_options(xynet INTERFACE "-Wall" INTERFACE "-fcoroutines" )

enable_testing()

add_subdirectory(${PROJECT_SOURCE_DIR}/test)
add_subdirectory(${PROJECT_SOURCE_DIR}/example)
