#pragma once
#include <le/vfs/uri.hpp>

namespace le {
struct Shader {
	Uri vertex{};
	Uri fragment{};

	explicit operator bool() const { return !vertex.is_empty() && !fragment.is_empty(); }
};
} // namespace le
