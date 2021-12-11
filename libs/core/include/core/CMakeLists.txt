target_sources(${PROJECT_NAME} PRIVATE
  array_map.hpp
  codepoint.hpp
  colour.hpp
  erased_ptr.hpp
  hash.hpp
  io.hpp
  log_channel.hpp
  log.hpp
  maths.hpp
  not_null.hpp
  nvec.hpp
  os.hpp
  ref.hpp
  services.hpp
  singleton.hpp
  span.hpp
  std_types.hpp
  time.hpp
  transform.hpp
  ubyte.hpp
  version.hpp
)

add_subdirectory(io)
add_subdirectory(utils)
