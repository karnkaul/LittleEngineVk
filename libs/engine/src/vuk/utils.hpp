#pragma once
#include <utility>
#include <set>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "core/std_types.hpp"
#include "common.hpp"
#include "info.hpp"

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
	std::set<vk::DynamicState> dynamicStates;
	f32 staticLineWidth = 1.0f;
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
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
};

struct BufferData final
{
	vk::DeviceSize size;
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags properties;
	VmaMemoryUsage vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
};

TResult<vk::Format> supportedFormat(PriorityList<vk::Format> const& desired, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

void wait(vk::Fence optional);
void waitAll(vk::ArrayProxy<const vk::Fence> validFences);

vk::DescriptorSetLayout createDescriptorSetLayout(u32 binding, u32 descriptorCount, vk::ShaderStageFlags stages);
void writeUniformDescriptor(Buffer buffer, vk::DescriptorSet descriptorSet, u32 binding);

Image createImage(ImageData const& data);
vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor,
							  vk::ImageViewType typev = vk::ImageViewType::e2D);

Buffer createBuffer(BufferData const& data);
void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, TransferOp* pOp);
bool writeToBuffer(Buffer buffer, void const* pData);

vk::Pipeline createPipeline(vk::PipelineLayout info, PipelineData const& data, vk::PipelineCache cache = vk::PipelineCache());

template <typename vkOwner = vk::Device, typename vkType>
void vkDestroy(vkType object)
{
	if constexpr (std::is_same_v<vkType, Buffer>)
	{
		vmaDestroyBuffer(g_info.vmaAllocator, object.buffer, object.handle);
	}
	else if constexpr (std::is_same_v<vkType, Image>)
	{
		vmaDestroyImage(g_info.vmaAllocator, object.image, object.handle);
	}
	else if constexpr (std::is_same_v<vkType, vk::DescriptorSetLayout>)
	{
		if (object != vk::DescriptorSetLayout())
		{
			g_info.device.destroyDescriptorSetLayout(object);
		}
	}
	else if constexpr (std::is_same_v<vkType, vk::DescriptorPool>)
	{
		if (object != vk::DescriptorPool())
		{
			g_info.device.destroyDescriptorPool(object);
		}
	}
	else if constexpr (std::is_same_v<vkOwner, vk::Instance>)
	{
		if (object != vkType() && g_info.instance != vk::Instance())
		{
			g_info.instance.destroy(object);
		}
	}
	else if constexpr (std::is_same_v<vkOwner, vk::Device>)
	{
		if (object != vkType() && g_info.device != vk::Device())
		{
			g_info.device.destroy(object);
		}
	}
	else
	{
		static_assert(FalseType<vkType>::value, "Unsupported vkOwner!");
	}
}

template <typename vkOwner = vk::Device, typename vkType, typename... vkTypes>
void vkDestroy(vkType object, vkTypes... objects)
{
	vkDestroy(object);
	vkDestroy(objects...);
}

inline void vkFree(vk::DeviceMemory memory)
{
	if (memory != vk::DeviceMemory())
	{
		g_info.device.freeMemory(memory);
	}
}

template <typename vkType, typename... vkTypes>
void vkFree(vkType memory0, vkTypes... memoryN)
{
	static_assert(std::is_same_v<vkType, vk::DeviceMemory>, "Invalid Type!");
	vkFree(memory0);
	vkFree(memoryN...);
}
} // namespace le::vuk
