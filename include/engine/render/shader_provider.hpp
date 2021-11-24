#pragma once
#include <core/io/path.hpp>
#include <graphics/render/pipeline_spec.hpp>

namespace le {
struct ShaderProvider {
	using Type = graphics::ShaderSpec::Type;

	io::Path uri;
	Type type = Type::eVertex;
};
} // namespace le
