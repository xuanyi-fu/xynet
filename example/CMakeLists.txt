add_library(example_common INTERFACE)
target_include_directories(example_common 
INTERFACE 
${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(simple)
add_subdirectory(ttcp)
add_subdirectory(pingpong)
add_subdirectory(chat)
add_subdirectory(file_transfer)