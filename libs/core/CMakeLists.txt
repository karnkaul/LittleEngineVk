project(levk-core)

# Library
add_library(${PROJECT_NAME})
add_library(levk::core ALIAS ${PROJECT_NAME})

# defines
target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_XYZW_ONLY GLM_FORCE_RADIANS GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_SILENT_WARNINGS GLM_ENABLE_EXPERIMENTAL GLM_EXT_INCLUDED)
target_compile_definitions(${PROJECT_NAME} PUBLIC KTL_FIXED_ANY_THROW)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    include
  PRIVATE
    src
)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    glm
    ktl::ktl
    dlog::dlog
    dtasks::dtasks
    djson::djson
    levk::interface
  PRIVATE
    levk::compile-options
    physfs-static
)
include(core_includes.cmake)
add_subdirectory(src)
target_source_group(${PROJECT_NAME})

# PCH
if(LEVK_USE_PCH)
  set(PRECOMPILE
    <string>
    <unordered_map>
    <vector>
  )
  target_precompile_headers(${PROJECT_NAME} PRIVATE ${PRECOMPILE})
endif()
