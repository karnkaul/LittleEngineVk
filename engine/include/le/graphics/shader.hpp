#pragma once
#include <le/vfs/uri.hpp>

namespace le {
struct Shader {
	Uri vertex{};
	Uri fragment{};
};
} // namespace le
