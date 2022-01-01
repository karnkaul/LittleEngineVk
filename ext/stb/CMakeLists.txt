project(stb-image)

unzip_archive(ARCHIVE stb_image.zip SUBDIR "${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.[hc]*")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
add_library(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(PLATFORM STREQUAL "Linux")
  target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
endif()
