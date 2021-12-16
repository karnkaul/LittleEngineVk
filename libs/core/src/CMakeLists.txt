target_sources(${PROJECT_NAME} PRIVATE
  colour.cpp
  io.cpp
  os.cpp
  time.cpp
  transform.cpp
  version.cpp
)

add_subdirectory(io)
add_subdirectory(utils)
