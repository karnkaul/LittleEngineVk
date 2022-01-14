set(target_name levk-engine)

add_library(${target_name} ${LEVK_LIBRARY_TYPE})
add_library(levk::${target_name} ALIAS ${target_name})
target_compile_definitions(${target_name} PUBLIC $<$<BOOL:${LEVK_EDITOR}>:LEVK_EDITOR>)
target_include_directories(${target_name} SYSTEM INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(${target_name} PRIVATE
  include
  src
  "${CMAKE_CURRENT_BINARY_DIR}/include"
)
target_link_libraries(${target_name}
  PUBLIC
    $<$<BOOL:${LEVK_USE_IMGUI}>:levk::dear-imgui>
    levk::levk-graphics
    levk::levk-window
    levk::levk-core
    dtasks::dtasks
    clap::clap
    dens::dens

  PRIVATE
    levk::levk-compile-options
    stb-image
    tinyobjloader
)
if(LEVK_USE_PCH)
  target_precompile_headers(${target_name} REUSE_FROM levk-graphics)
endif()

set(version_file "${CMAKE_CURRENT_BINARY_DIR}/include/levk/engine/build_version.hpp")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/build_version.hpp.in" "${version_file}")

file(GLOB_RECURSE levk_includes CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.?pp")
file(GLOB_RECURSE levk_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.?pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${levk_includes} ${levk_sources})
source_group(TREE "${CMAKE_CURRENT_BINARY_DIR}" FILES ${version_file})
target_sources(${target_name} PRIVATE ${levk_includes} ${levk_sources} ${version_file})

if(LEVK_INSTALL)
  install(TARGETS levk-engine EXPORT levk-engine-targets)
  install(EXPORT levk-engine-targets
    FILE levk-engine-targets.cmake
    NAMESPACE levk::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/levk
  )
  install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
  )
endif()
