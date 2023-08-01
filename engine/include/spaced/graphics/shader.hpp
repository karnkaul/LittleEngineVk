#pragma once
#include <spaced/vfs/uri.hpp>

namespace spaced {
struct Shader {
	Uri vertex{};
	Uri fragment{};
};
} // namespace spaced
