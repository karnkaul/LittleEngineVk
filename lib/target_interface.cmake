add_library(levk-compile-options INTERFACE)
add_library(levk::compile-options ALIAS levk-compile-options)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(levk-compile-options INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type
  )

  if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    # target_compile_options(levk-compile-options INTERFACE -fmodules-ts)
  endif()
endif()

if(CMAKE_GENERATOR MATCHES "^(Visual Studio)")
  target_compile_options(levk-compile-options INTERFACE /MP)
endif()
