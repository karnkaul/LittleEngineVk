if(LE_USE_FREETYPE)
  target_sources(${PROJECT_NAME} PRIVATE
    library.cpp
    library.hpp
  )
endif()
