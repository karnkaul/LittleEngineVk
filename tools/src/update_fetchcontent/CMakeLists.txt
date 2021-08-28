cmake_minimum_required(VERSION 3.14)

project(update-fetchcontent)

find_package(clap REQUIRED)
add_executable(update-fetchcontent)
target_compile_features(update-fetchcontent PRIVATE cxx_std_20)
target_sources(update-fetchcontent PRIVATE main.cpp)
target_link_libraries(update-fetchcontent PRIVATE clap::clap)
if(UNIX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
	target_compile_options(update-fetchcontent PRIVATE -Wall -Wextra)
endif()

install(TARGETS update-fetchcontent RUNTIME DESTINATION .)
