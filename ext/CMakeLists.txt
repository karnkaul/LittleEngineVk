# external
set(PHYSFS_BUILD_STATIC ON)
set(PHYSFS_BUILD_SHARED OFF)
set(PHYSFS_BUILD_DOCS OFF)
set(PHYSFS_ARCHIVE_GRP OFF)
set(PHYSFS_ARCHIVE_HOG OFF)
set(PHYSFS_ARCHIVE_ISO9660 OFF)
set(PHYSFS_ARCHIVE_MVL OFF)
set(PHYSFS_ARCHIVE_QPAK OFF)
set(PHYSFS_ARCHIVE_SLB OFF)
set(PHYSFS_ARCHIVE_VDF OFF)
set(PHYSFS_ARCHIVE_WAD OFF)
set(PHYSFS_ARCHIVE_ZIP ON)
set(PHYSFS_BUILD_TEST OFF)
add_subdirectory(physfs)
target_include_directories(physfs-static SYSTEM INTERFACE 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/physfs/src>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}
)

add_subdirectory(glm)
target_compile_options(glm INTERFACE 
  $<$<OR:$<BOOL:${LINUX_GCC}>,$<BOOL:${WINDOWS_GCC}>>:-Wno-volatile> 
  $<$<OR:$<BOOL:${LINUX_CLANG}>,$<BOOL:${WINDOWS_CLANG}>>:-Wno-deprecated-volatile>
)

if(LEVK_USE_GLFW)
  set(GLFW_VULKAN_STATIC OFF)
  set(GLFW_INSTALL ${LEVK_INSTALL})
  set(GLFW_BUILD_DOCS OFF)
  set(GLFW_BUILD_EXAMPLES OFF)
  set(GLFW_BUILD_TESTS OFF)
  add_subdirectory(glfw)
  if(WINDOWS_CLANG)
    target_compile_options(glfw PRIVATE -w)
  endif()
  if(LEVK_INSTALL)
    export(EXPORT glfwTargets)
  endif()
endif()

add_subdirectory(vma)

add_subdirectory(stb)

add_subdirectory(tinyobjloader)

if(LEVK_USE_IMGUI)
  add_subdirectory(dear_imgui)
endif()

set(SPIRV_CROSS_CLI OFF)
set(SPIRV_CROSS_ENABLE_CPP OFF)
set(SPIRV_CROSS_ENABLE_C_API OFF)
set(SPIRV_CROSS_ENABLE_GLSL ON)
set(SPIRV_CROSS_ENABLE_HLSL OFF)
set(SPIRV_CROSS_ENABLE_MSL OFF)
set(SPIRV_CROSS_ENABLE_REFLECT OFF)
set(SPIRV_CROSS_ENABLE_TESTS OFF)
set(SPIRV_CROSS_ENABLE_UTIL OFF)
set(SPIRV_CROSS_SHARED OFF)
set(SPIRV_CROSS_STATIC ON)
add_subdirectory(spirv_cross)
if(LINUX_CLANG OR LINUX_GCC OR WINDOWS_CLANG OR WINDOWS_GCC)
  target_compile_options(spirv-cross-core PRIVATE -w)
  target_compile_options(spirv-cross-glsl PRIVATE -w)
  target_include_directories(spirv-cross-glsl SYSTEM INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/spirv_cross>)
elseif(PLATFORM STREQUAL Win64)
  target_compile_definitions(spirv-cross-core PUBLIC _CRT_SECURE_NO_WARNINGS NO_MIN_MAX)
endif()

add_subdirectory(freetype)
