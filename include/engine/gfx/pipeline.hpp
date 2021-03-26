#pragma once
#include <memory>
#include <string>
#include <engine/resources/resource_types.hpp>
#include <kt/enum_flags/enum_flags.hpp>

namespace le::gfx {
struct Pipeline final {
	enum class Polygon : s8 { eFill, eLine, eCOUNT_ };
	enum class Cull : s8 { eNone, eFront, eBack, eCOUNT_ };
	enum class FFace : s8 { eCounterClockwise, eClockwise, eCOUNT_ };
	enum class Flag { eBlend, eDepthTest, eDepthWrite, eCOUNT_ };
	using Flags = kt::enum_flags<Flag>;

	res::Shader shader;
	f32 lineWidth = 1.0f;
	Polygon polygonMode = Polygon::eFill;
	Cull cullMode = Cull::eNone;
	FFace frontFace = FFace::eCounterClockwise;
	Flags flags = Flags(Flag::eBlend) | Flag::eDepthTest | Flag::eDepthWrite;
};
} // namespace le::gfx
