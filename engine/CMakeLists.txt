add_library(spaced-engine)
add_library(spaced::spaced-engine ALIAS spaced-engine)

target_compile_definitions(spaced-engine PUBLIC
  $<$<CONFIG:Debug>:SPACED_DEBUG>
)

target_compile_definitions(spaced-engine PRIVATE
  $<$<BOOL:${SPACED_PCH}>:SPACED_PCH>
  SPACED_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(spaced-engine PUBLIC
  spaced::spaced-ext
)

target_include_directories(spaced-engine PUBLIC
  include
)

target_include_directories(spaced-engine PRIVATE
  .
)

target_compile_features(spaced-engine PUBLIC
  cxx_std_20
)

add_subdirectory(src)

include(header_list.cmake)

target_sources(spaced-engine PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
  ${header_list}
)

if(SPACED_PCH)
  target_precompile_headers(spaced-engine PRIVATE
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
