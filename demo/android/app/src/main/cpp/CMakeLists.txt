cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_EXTENSIONS OFF)

project(levk-demo)

set(LEVK_VULKAN_DYNAMIC ON)
add_subdirectory(../../../../../.. "${CMAKE_CURRENT_BINARY_DIR}/levk_engine")
add_subdirectory(../../../../../lib "${CMAKE_CURRENT_BINARY_DIR}/levk_demo_lib")

add_library(levk-demo SHARED main.cpp)
target_link_libraries(levk-demo PRIVATE levk::demo::lib)
target_link_options(levk-demo PRIVATE -u ANativeActivity_onCreate)
