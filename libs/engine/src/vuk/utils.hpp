#pragma once
#include <utility>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "core/std_types.hpp"
#include "common.hpp"

namespace le::vuk
{
struct PipelineData final
{
	class Shader const* pShader = nullptr;
	vk::RenderPass renderPass;
	vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
	vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eNone;
	vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
	vk::ColorComponentFlags colourWriteMask =
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	f32 lineWidth = 1.0f;
	bool bBlend = false;
};

struct TransferOp final
{
	vk::Queue queue;
	vk::CommandPool pool;

	vk::Fence transferred;
	vk::CommandBuffer commandBuffer;
};

struct ImageData final
{
	vk::Extent3D size;
	vk::Format format;
	vk::ImageTiling tiling;
	vk::ImageUsageFlags usage;
	vk::MemoryPropertyFlags properties;
	vk::ImageType type = vk::ImageType::e2D;
	Info::QFlags queueFlags = Info::QFlags(Info::QFlag::eGraphics, Info::QFlag::eTransfer);
};

struct BufferData final
{
	vk::DeviceSize size;
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags properties;
};

void wait(vk::Fence optional);
void waitAll(vk::ArrayProxy<const vk::Fence> validFences);

VkResource<vk::Image> createImage(ImageData const& data);
vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor,
							  vk::ImageViewType typev = vk::ImageViewType::e2D);

VkResource<vk::Buffer> createBuffer(BufferData const& data);
void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, TransferOp* pOp);

vk::RenderPass createRenderPass(vk::Format format);
vk::Pipeline createPipeline(vk::PipelineLayout info, PipelineData const& data, vk::PipelineCache cache = vk::PipelineCache());

bool writeToBuffer(VkResource<vk::Buffer> buffer, void const* pData, vk::DeviceSize size = 0, vk::DeviceSize offset = 0);
} // namespace le::vuk
