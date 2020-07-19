#pragma once
#include <memory>
#include <string>
#include <core/flags.hpp>
#include <engine/resources/resource_types.hpp>

namespace le::gfx
{
enum class PolygonMode : s8
{
	eFill,
	eLine,
	eCOUNT_
};

enum class CullMode : s8
{
	eNone,
	eFront,
	eBack,
	eCOUNT_
};

enum class FrontFace : s8
{
	eCounterClockwise,
	eClockwise,
	eCOUNT_
};

class Pipeline final
{
public:
	enum class Flag
	{
		eBlend,
		eDepthTest,
		eDepthWrite,
		eCOUNT_
	};
	using Flags = TFlags<Flag>;

	struct Info final
	{
		std::string name;
		resources::Shader shader;
		f32 lineWidth = 1.0f;
		PolygonMode polygonMode = PolygonMode::eFill;
		CullMode cullMode = CullMode::eNone;
		FrontFace frontFace = FrontFace::eCounterClockwise;
		Flags flags = Flag::eBlend | Flag::eDepthTest | Flag::eDepthWrite;
	};

public:
	std::unique_ptr<class PipelineImpl> m_uImpl;

public:
	Pipeline();
	Pipeline(Pipeline&&);
	Pipeline& operator=(Pipeline&&);
	~Pipeline();

public:
	std::string const& name() const;
};
} // namespace le::gfx
