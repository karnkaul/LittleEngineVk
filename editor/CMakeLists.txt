project(levk-editor)

option(LEVK_EDITOR_REUSE_PCH "Reuse PCH from levk" ON)

add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
  levk::lib
  levk::importer
  levk::compile-options
)

if(LEVK_EDITOR_REUSE_PCH)
  target_precompile_headers(${PROJECT_NAME} REUSE_FROM levk-lib)
endif()

target_sources(${PROJECT_NAME} PRIVATE
  src/app.cpp
  src/app.hpp
  src/asset_tree.cpp
  src/asset_tree.hpp
  src/loader.hpp
  src/main.cpp
)
