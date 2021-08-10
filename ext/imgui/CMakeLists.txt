project(imgui)

unzip_archive(imgui_1.83.zip "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.[hc]*")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_library(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${Vulkan_INCLUDE_DIRS}"
)
target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES})
if(LEVK_USE_GLFW)
	target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
endif()
if(PLATFORM STREQUAL "Linux")
	target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
endif()
