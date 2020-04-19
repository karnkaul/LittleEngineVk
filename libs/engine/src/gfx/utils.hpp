#pragma once
#include <utility>
#include <set>
#include <glm/glm.hpp>
#include "core/std_types.hpp"
#include "core/utils.hpp"
#include "common.hpp"
#include "info.hpp"

namespace le::gfx
{
TResult<vk::Format> supportedFormat(PriorityList<vk::Format> const& desired, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

void waitFor(vk::Fence optional);
void waitAll(vk::ArrayProxy<vk::Fence const> validFences);

bool isSignalled(vk::Fence fence);
bool allSignalled(ArrayView<vk::Fence const> fences);

vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor,
							  vk::ImageViewType type = vk::ImageViewType::e2D);

template <typename vkOwner = vk::Device, typename vkType>
void vkDestroy(vkType object)
{
	if constexpr (std::is_same_v<vkType, vk::DescriptorSetLayout>)
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
	else if constexpr (std::is_same_v<vkType, vk::ImageView>)
	{
		if (object != vk::ImageView())
		{
			g_info.device.destroyImageView(object);
		}
	}
	else if constexpr (std::is_same_v<vkType, vk::Sampler>)
	{
		if (object != vk::Sampler())
		{
			g_info.device.destroySampler(object);
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
} // namespace le::gfx
