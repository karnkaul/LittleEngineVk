include(target_interface.cmake)

add_subdirectory(levk)

if(LEVK_BUILD_IMPORTER)
  add_subdirectory(importer)
endif()
