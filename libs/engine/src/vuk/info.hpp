#pragma once
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"
#include "vuk/common.hpp"

namespace le::vuk
{
struct Info final
{
	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::DescriptorSetLayout matricesLayout;

	struct
	{
		u32 graphics;
		u32 present;
		u32 transfer;
	} queueFamilyIndices;

	struct
	{
		vk::Queue graphics;
		vk::Queue present;
		vk::Queue transfer;
	} queues;

	bool isValid(vk::SurfaceKHR surface) const;
	std::vector<u32> uniqueQueueIndices(bool bPresent, bool bTransfer) const;
	vk::SharingMode sharingMode(bool bPresent, bool bTransfer) const;
	u32 findMemoryType(u32 typeFilter, vk::MemoryPropertyFlags properties) const;
};

inline Info g_info;

struct Service final
{
	Service(InitData const& initData);
	~Service();
};

template <typename vkOwner = vk::Device, typename vkType>
void vkDestroy(vkType object)
{
	if constexpr (std::is_same_v<vkType, VkResource<vk::Buffer>>)
	{
		if (object.resource != vk::Buffer())
		{
			g_info.device.destroyBuffer(object.resource);
		}
		if (object.memory != vk::DeviceMemory())
		{
			g_info.device.freeMemory(object.memory);
		}
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

template <typename vkType>
void vkFree(vkType memory)
{
	if (memory != vkType())
	{
		g_info.device.freeMemory(memory);
	}
}

template <typename vkType, typename... vkTypes>
void vkFree(vkType memory0, vkTypes... memoryN)
{
	free(memory0);
	free(memoryN...);
}
} // namespace le::vuk
