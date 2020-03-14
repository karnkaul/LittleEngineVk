#pragma once
#include <functional>
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

	struct
	{
		u32 graphics;
		u32 present;
	} queueFamilyIndices;

	struct
	{
		std::vector<vk::Queue> graphics;
		vk::Queue present;
	} queues;

	bool isValid(vk::SurfaceKHR surface) const;

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
	if constexpr (std::is_same_v<vkOwner, vk::Instance>)
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
} // namespace le::vuk
