#pragma once
#include <memory>

namespace le::gfx
{
class Shader;

enum class PolygonMode : u8
{
	eFill,
	eLine,
	eCOUNT_
};

enum class CullMode : u8
{
	eNone,
	eCounterClockwise,
	eClockwise,
	eCOUNT_
};

enum class FrontFace : u8
{
	eFront,
	eBack,
	eCOUNT_
};

class Pipeline final
{
public:
	struct Info final
	{
		std::string name;
		Shader* pShader = nullptr;
		f32 lineWidth = 1.0f;
		PolygonMode polygonMode = PolygonMode::eFill;
		CullMode cullMode = CullMode::eNone;
		FrontFace frontFace = FrontFace::eFront;
		bool bBlend = true;
	};

public:
	std::unique_ptr<class PipelineImpl> m_uImpl;
};
} // namespace le::gfx
