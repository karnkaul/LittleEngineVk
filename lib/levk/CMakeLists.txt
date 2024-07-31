project(levk-lib VERSION ${build_version})

add_library(${PROJECT_NAME})
add_library(levk::lib ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
  include
  "${CMAKE_CURRENT_BINARY_DIR}/include"

  PRIVATE
  src
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  levk::ext

  PRIVATE
  levk::compile-options
)

set(version_header_path "${CMAKE_CURRENT_BINARY_DIR}/include/levk/build_version.hpp")
configure_file(build_version.hpp.in "${version_header_path}" @ONLY)

file(GLOB_RECURSE sources LIST_DIRECTORIES false CONFIGURE_DEPENDS "include/*.hpp" "src/*.[hc]pp")
target_sources(${PROJECT_NAME} PRIVATE
  ${sources}
  "${version_header_path}"
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
  <vulkan/vulkan.hpp>
  <vk_mem_alloc.h>
  <algorithm>
  <atomic>
  <chrono>
  <cmath>
  <cstdint>
  <deque>
  <expected>
  <filesystem>
  <format>
  <functional>
  <future>
  <map>
  <memory>
  <mutex>
  <numeric>
  <optional>
  <print>
  <ranges>
  <span>
  <string>
  <thread>
  <unordered_map>
  <unordered_set>
  <utility>
  <variant>
  <vector>
)
