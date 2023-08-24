add_library(${PROJECT_NAME})
add_library(le::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC
  $<$<CONFIG:Debug>:LE_DEBUG>
  $<$<BOOL:${LE_PCH}>:LE_PCH>
  $<$<BOOL:${LE_USE_FREETYPE}>:LE_USE_FREETYPE>
  $<$<BOOL:${CAPO_USE_OPENAL}>:CAPO_USE_OPENAL>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
  le::le-ext
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  le::le-compile-options
)

target_include_directories(${PROJECT_NAME} PUBLIC
  include
  "${CMAKE_CURRENT_BINARY_DIR}/include"
)

target_include_directories(${PROJECT_NAME} PRIVATE
  .
)

target_compile_features(${PROJECT_NAME} PUBLIC
  cxx_std_20
)

configure_file(src/build_version.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/include/le/build_version.hpp")

add_subdirectory(src)

include(header_list.cmake)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
  ${header_list}
)

if(LE_PCH)
  target_precompile_headers(${PROJECT_NAME} PRIVATE
    <vulkan/vulkan.hpp>
    <algorithm>
    <atomic>
    <chrono>
    <cstdint>
    <deque>
    <filesystem>
    <format>
    <functional>
    <future>
    <memory>
    <mutex>
    <optional>
    <span>
    <string>
    <unordered_map>
    <unordered_set>
    <variant>
    <vector>
  )
endif()
