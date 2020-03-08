#pragma once
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"

namespace le::vuk
{
class Pipeline
{
public:
	struct State
	{
	};

	struct Data
	{
		State state;
		std::vector<vk::PipelineShaderStageCreateInfo> shaders;
		vk::RenderPass renderPass;
		vk::Viewport viewport;
		vk::Rect2D scissor;
		vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
		vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eBack;
		vk::FrontFace frontFace = vk::FrontFace::eClockwise;
		vk::ColorComponentFlags colourWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
												  | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		f32 lineWidth = 1.0f;
		bool bBlend = false;
	};

private:
	vk::PipelineLayout m_layout;
	vk::Pipeline m_pipeline;

public:
	Pipeline(Data const& data);
	~Pipeline();
};
} // namespace le::vuk
