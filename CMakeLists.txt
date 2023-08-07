cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(little-engine VERSION 0.1.0)

set(is_root_project OFF)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  set(is_root_project ON)
endif()

option(LE_PCH "Use PCH for little" ON)
option(LE_BUILD_SCENE "Build little-engine scene library" ON)
option(LE_BUILD_TOOLS "Build little-engine tools" ${is_root_project})
option(LE_USE_FREETYPE "Use Freetype as the font backend" ON)
option(LE_BUILD_EXAMPLE "Build little-engine example" ${is_root_project})

add_library(le-compile-options INTERFACE)
add_library(le::le-compile-options ALIAS le-compile-options)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(le-compile-options INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Wunused -Werror=return-type $<$<NOT:$<CONFIG:Debug>>:-Werror>
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
  target_compile_options(le-compile-options INTERFACE
    $<$<NOT:$<CONFIG:Debug>>:/WX>
  )
endif()

add_subdirectory(ext)
add_subdirectory(engine)

if(LE_BUILD_SCENE)
  add_subdirectory(scene)
endif()

if(LE_BUILD_TOOLS)
  add_subdirectory(tools)
endif()

if(LE_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()
