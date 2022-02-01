project(levk-gameplay)

add_library(${PROJECT_NAME})
add_library(levk::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(${PROJECT_NAME} PRIVATE
  include
  src
)
target_link_libraries(${PROJECT_NAME}
  PUBLIC
  levk::levk-engine

  PRIVATE
  levk::levk-compile-options
)

if(LEVK_USE_PCH)
  target_precompile_headers(${PROJECT_NAME} REUSE_FROM levk-engine)
endif()

file(GLOB_RECURSE includes CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.[hc]pp")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.[hc]pp")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${includes} ${sources})
target_sources(${PROJECT_NAME} PRIVATE ${includes} ${sources})

if(LEVK_INSTALL)
  install(TARGETS levk-gameplay EXPORT levk-gameplay-targets)
  install(EXPORT levk-gameplay-targets
    FILE levk-gameplay-targets.cmake
    NAMESPACE levk::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/levk
  )
  install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
  )
endif()
