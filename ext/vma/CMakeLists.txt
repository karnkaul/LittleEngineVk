project(vma)

unzip_archive(ARCHIVE vma.zip SUBDIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC 
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(${PROJECT_NAME} PUBLIC dyvk::dyvk)
target_sources(${PROJECT_NAME} PRIVATE include/vk_mem_alloc.h src/vk_mem_alloc.cpp)
target_source_group(TARGET ${PROJECT_NAME})

if(LEVK_INSTALL)
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h*"
  )
endif()
