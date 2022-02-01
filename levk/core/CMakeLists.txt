project(levk-core VERSION ${build_version})

# Library
add_library(${PROJECT_NAME})
add_library(levk::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# defines
target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_XYZW_ONLY GLM_FORCE_RADIANS GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_SILENT_WARNINGS GLM_ENABLE_EXPERIMENTAL GLM_EXT_INCLUDED)
target_compile_definitions(${PROJECT_NAME} PUBLIC KTL_FIXED_ANY_THROW)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    src
    "${CMAKE_CURRENT_BINARY_DIR}/include"
)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
    glm
    ktl::ktl
    dlog::dlog
    dtasks::dtasks
    djson::djson
    levk::levk-build-interface
  PRIVATE
    levk::levk-compile-options
    physfs-static
)
include(core_includes.cmake)
add_subdirectory(src)
target_source_group(TARGET ${PROJECT_NAME})

set(version_file "${CMAKE_CURRENT_BINARY_DIR}/include/levk/core/build_version.hpp")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/build_version.hpp.in" "${version_file}")
source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${version_file})

# PCH
if(LEVK_USE_PCH)
  set(PRECOMPILE
    <string>
    <unordered_map>
    <vector>
  )
  target_precompile_headers(${PROJECT_NAME} PRIVATE ${PRECOMPILE})
endif()

if(LEVK_INSTALL)
  install(TARGETS levk-core EXPORT levk-core-targets)
  install(EXPORT levk-core-targets
    FILE levk-core-targets.cmake
    NAMESPACE levk::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/levk
  )
  install(DIRECTORY include/ DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
  )
endif()
