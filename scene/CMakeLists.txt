project(le-scene)

add_library(${PROJECT_NAME})
add_library(le::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC
  le::little-engine
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  le::le-compile-options
)

target_include_directories(${PROJECT_NAME} PUBLIC
  include
)

target_compile_features(${PROJECT_NAME} PUBLIC
  cxx_std_20
)

add_subdirectory(src)

include(header_list.cmake)

target_sources(${PROJECT_NAME} PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
  ${header_list}
)

if(LE_PCH)
  target_precompile_headers(${PROJECT_NAME} REUSE_FROM little-engine)
endif()
