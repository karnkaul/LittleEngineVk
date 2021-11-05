project(vk-hpp)

set(archive_name vulkan_hpp.zip)

unzip_archive(${archive_name} "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(${PROJECT_NAME})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE include/vulkan/vulkan.h include/vulkan/vulkan.hpp src/impl.cpp)
target_source_group(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC include)
target_compile_definitions(${PROJECT_NAME} PUBLIC VK_NO_PROTOTYPES)
