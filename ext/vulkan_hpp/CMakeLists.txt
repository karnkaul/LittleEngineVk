project(levk-vulkan-hpp)

# Vulkan
if(NOT LEVK_VULKAN_DYNAMIC)
	find_package(Vulkan)
	if(NOT Vulkan_FOUND)
		message(FATAL_ERROR 
"Vulkan not found and LEVK_VULKAN_DYNAMIC not set!

Note: for custom SDK locations, define `VULKAN_SDK` and source `vk-setup-env.sh` into this shell (and runtime environment).
Make sure to delete CMakeCache.txt before re-configuring!"
		)
	endif()
endif()

unzip_archive(vulkan_hpp.zip "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h*")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_library(${PROJECT_NAME} INTERFACE)
add_library(levk::vulkan-hpp ALIAS ${PROJECT_NAME})
target_sources(${PROJECT_NAME} INTERFACE ${SOURCES})
target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<STREQUAL:${PLATFORM},Android>:VK_USE_PLATFORM_ANDROID_KHR>)
target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Re-route INCLUDE_DIRS to vendored headers
set(Vulkan_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE STRING "" FORCE)
if(LEVK_VULKAN_DYNAMIC)
	set(Vulkan_LIBRARIES "" CACHE STRING "" FORCE)
	set(VK_LINK "[Dynamic]")
else()
	set(VK_LINK "[Static]")
endif()

target_link_libraries(${PROJECT_NAME} INTERFACE ${Vulkan_LIBRARIES})
# Dynamic loader
target_compile_definitions(${PROJECT_NAME} INTERFACE 
	$<$<BOOL:${LEVK_VULKAN_DYNAMIC}>:
		VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
		VK_NO_PROTOTYPES
	>
)

message(STATUS "\n== Vulkan ${VK_LINK} ==\n\tHeaders      : ${Vulkan_INCLUDE_DIRS}\n\tLibraries    : ${Vulkan_LIBRARIES}\n")
