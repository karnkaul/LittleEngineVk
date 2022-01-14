project(stb-image)

unzip_archive(ARCHIVE stb_image.zip SUBDIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC 
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_sources(${PROJECT_NAME} PRIVATE include/stb/stb_image.h src/stb_image.c)
target_source_group(TARGET ${PROJECT_NAME})
