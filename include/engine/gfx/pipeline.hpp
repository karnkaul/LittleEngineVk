#pragma once
#include <memory>
#include <string>
#include <core/flags.hpp>
#include <engine/resources/resource_types.hpp>

namespace le::gfx
{
struct Pipeline final
{
	enum class Polygon : s8
	{
		eFill,
		eLine,
		eCOUNT_
	};
	enum class Cull : s8
	{
		eNone,
		eFront,
		eBack,
		eCOUNT_
	};
	enum class FFace : s8
	{
		eTCounterClockwise,
		eClockwise,
		eCOUNT_
	};
	enum class Flag
	{
		eBlend,
		eDepthTest,
		eDepthWrite,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	res::Shader shader;
	f32 lineWidth = 1.0f;
	Polygon polygonMode = Polygon::eFill;
	Cull cullMode = Cull::eNone;
	FFace frontFace = FFace::eTCounterClockwise;
	Flags flags = Flag::eBlend | Flag::eDepthTest | Flag::eDepthWrite;
};
} // namespace le::gfx
