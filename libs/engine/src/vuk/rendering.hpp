#pragma once
#include <utility>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "core/std_types.hpp"

namespace le::vuk
{
struct RenderPassData
{
	vk::Format format;
};

struct PipelineData
{
	class Shader const* pShader = nullptr;
	vk::RenderPass renderPass;
	vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
	vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eBack;
	vk::FrontFace frontFace = vk::FrontFace::eClockwise;
	vk::ColorComponentFlags colourWriteMask =
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	f32 lineWidth = 1.0f;
	bool bBlend = false;
};

struct ImageData
{
	glm::ivec2 size = {};
	vk::Format format;
	vk::ImageTiling tiling;
	vk::ImageUsageFlags usage;
	vk::MemoryPropertyFlags properties;
};

std::pair<vk::Image, vk::DeviceMemory> createImage(ImageData const& data);
vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);

vk::RenderPass createRenderPass(RenderPassData const& data);
vk::Pipeline createPipeline(vk::PipelineLayout info, PipelineData const& data, vk::PipelineCache cache = vk::PipelineCache());
} // namespace le::vuk
