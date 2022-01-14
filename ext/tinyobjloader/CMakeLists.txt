project(tinyobjloader)

unzip_archive(ARCHIVE tinyobjloader.zip SUBDIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC 
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_sources(${PROJECT_NAME} PRIVATE include/tinyobjloader/tiny_obj_loader.h src/tiny_obj_loader.cpp)
target_source_group(TARGET ${PROJECT_NAME})
