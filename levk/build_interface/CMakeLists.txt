add_library(levk-build-interface INTERFACE)
add_library(levk::levk-build-interface ALIAS levk-build-interface)
target_compile_definitions(levk-build-interface INTERFACE
  _UNICODE
  $<$<NOT:$<CONFIG:Debug>>:NDEBUG LEVK_RELEASE>
  $<$<CONFIG:Debug>:LEVK_DEBUG>
  $<$<BOOL:${MSVC_RUNTIME}>:WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS>
  $<$<BOOL:${LEVK_PRE_RELEASE}>:LEVK_PRE_RELEASE>
)
target_compile_features(levk-build-interface INTERFACE cxx_std_20)

add_library(levk-compile-options INTERFACE)
add_library(levk::levk-compile-options ALIAS levk-compile-options)

if(LINUX_GCC OR LINUX_CLANG OR WINDOWS_GCC OR WINDOWS_CLANG)
  target_compile_options(levk-compile-options INTERFACE
    -Werror=return-type
    $<$<NOT:$<BOOL:${WINDOWS_CLANGCL}>>:-Wall -Wextra>
    $<$<NOT:$<CONFIG:Debug>>:-Werror>
    $<$<NOT:$<BOOL:${WINDOWS_CLANG}>>:-fexceptions>
    $<$<OR:$<BOOL:${LINUX_GCC}>,$<BOOL:${WINDOWS_GCC}>>:-utf-8>
    $<$<OR:$<BOOL:${LEVK_ASAN}>,$<BOOL:${LEVK_UBSAN}>>:-O1 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls>
    $<$<BOOL:${LEVK_ASAN}>:-fsanitize=address>
    $<$<BOOL:${LEVK_UBSAN}>:-fsanitize=undefined>
  )
elseif(WINDOWS_MSBUILD)
  target_compile_options(levk-compile-options INTERFACE
    $<$<NOT:$<CONFIG:Debug>>:/Oi /WX>
    /MP
  )
endif()

if(WINDOWS_CLANGCL)
  target_compile_options(levk-compile-options INTERFACE -utf-8)
endif()

add_library(levk-link-options INTERFACE)
add_library(levk::levk-link-options ALIAS levk-link-options)

if(PLATFORM STREQUAL "Linux")
  target_link_options(levk-link-options INTERFACE
    -Wl,-z,origin # Allow $ORIGIN in RUNPATH
    $<$<BOOL:${LEVK_ASAN}>:-fsanitize=address>
    $<$<BOOL:${LEVK_UBSAN}>:-fsanitize=undefined>
  )
elseif(PLATFORM STREQUAL "Win64" AND NOT WINDOWS_GCC)
  if(MSVC)
    target_link_options(levk-link-options INTERFACE
      /ENTRY:mainCRTStartup # Link to main() and not WinMain()
      /SUBSYSTEM:$<IF:$<CONFIG:Debug>,CONSOLE,WINDOWS> # Spawn a console in Debug
    )
  else()
    target_link_options(levk-link-options INTERFACE -Wl,/SUBSYSTEM:$<IF:$<CONFIG:Debug>,CONSOLE,WINDOWS>,/ENTRY:mainCRTStartup)
  endif()
endif()

install(TARGETS levk-build-interface levk-compile-options levk-link-options EXPORT levk-build-interface-targets)
install(EXPORT levk-build-interface-targets
  FILE levk-build-interface-targets.cmake
  NAMESPACE levk::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/levk
)
