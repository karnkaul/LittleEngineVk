#include <spaced/engine/build_version.hpp>

auto spaced::build_version() -> std::string_view { return SPACED_VERSION; }
