project(vma)

unzip_archive(vma.zip "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.[hc]*")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_library(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} PUBLIC vulkan-hpp)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<STREQUAL:${PLATFORM},Android>:VK_NO_PROTOTYPES>)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<STREQUAL:${PLATFORM},Linux>:-fPIC>)
