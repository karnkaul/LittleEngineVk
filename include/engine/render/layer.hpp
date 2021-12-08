#pragma once
#include <graphics/render/pipeline_flags.hpp>

namespace le {
enum class PolygonMode { eFill, eLine, ePoint, eCOUNT_ };
enum class Topology { ePointList, eLineList, eLineStrip, eTriangleList, eTriangleStrip, eTriangleFan, eCOUNT_ };
using RenderFlag = graphics::PFlag;
using RenderFlags = graphics::PFlags;
constexpr auto rflags_all = graphics::pflags_all;

struct RenderLayer {
	PolygonMode mode = PolygonMode::eFill;
	Topology topology = Topology::eTriangleList;
	RenderFlags flags = rflags_all;
	s64 order = 0;
	f32 lineWidth = 1.0f;

	constexpr bool operator==(RenderLayer const& rhs) const = default;
};
} // namespace le