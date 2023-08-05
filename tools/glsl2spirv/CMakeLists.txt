cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(glsl2spirv)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE glsl2spirv.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE
  le::le-compile-options
  clap::clap
)
