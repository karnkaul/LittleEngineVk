#pragma once
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "core/log_config.hpp"
#include "core/std_types.hpp"
#include "core/flags.hpp"
#include "common.hpp"

namespace le::gfx
{
struct Instance final
{
	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceLimits deviceLimits;

	f32 lineWidthMin = 1.0f;
	f32 lineWidthMax = 1.0f;
	log::Level validationLog = log::Level::eWarning;

	u32 findMemoryType(u32 typeFilter, vk::MemoryPropertyFlags properties) const;
	f32 lineWidth(f32 desired) const;

	TResult<vk::Format> supportedFormat(PriorityList<vk::Format> const& desired, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

	template <typename vkType>
	void destroy(vkType object) const;
};

struct Device final
{
	struct
	{
		Queue graphics;
		Queue present;
		Queue transfer;
	} queues;

	vk::Device device;

	bool isValid(vk::SurfaceKHR surface) const;
	UniqueQueues uniqueQueues(QFlags flags) const;

	void waitIdle() const;

	vk::Fence createFence(bool bSignalled) const;
	void waitFor(vk::Fence optional) const;
	void waitAll(vk::ArrayProxy<vk::Fence const> validFences) const;
	void resetFence(vk::Fence optional) const;
	void resetAll(vk::ArrayProxy<vk::Fence const> validFences) const;

	bool isSignalled(vk::Fence fence) const;
	bool allSignalled(ArrayView<vk::Fence const> fences) const;

	vk::ImageView createImageView(ImageViewInfo const& info);

	template <typename vkType>
	void destroy(vkType object) const;

	template <typename vkType, typename... vkTypes>
	void destroy(vkType object, vkTypes... objects) const;
};

inline Instance g_instance;
inline Device g_device;

struct Service final
{
	Service(InitInfo const& info);
	~Service();
};

template <typename vkType>
void Instance::destroy(vkType object) const
{
	if (object != vkType() && instance != vk::Instance())
	{
		instance.destroy(object);
	}
}

template <typename vkType>
void Device::destroy(vkType object) const
{
	if (device != vk::Device())
	{
		if constexpr (std::is_same_v<vkType, vk::DescriptorSetLayout>)
		{
			if (object != vk::DescriptorSetLayout())
			{
				device.destroyDescriptorSetLayout(object);
			}
		}
		else if constexpr (std::is_same_v<vkType, vk::DescriptorPool>)
		{
			if (object != vk::DescriptorPool())
			{
				device.destroyDescriptorPool(object);
			}
		}
		else if constexpr (std::is_same_v<vkType, vk::ImageView>)
		{
			if (object != vk::ImageView())
			{
				device.destroyImageView(object);
			}
		}
		else if constexpr (std::is_same_v<vkType, vk::Sampler>)
		{
			if (object != vk::Sampler())
			{
				device.destroySampler(object);
			}
		}
		else
		{
			if (object != vkType())
			{
				device.destroy(object);
			}
		}
	}
	return;
}

template <typename vkType, typename... vkTypes>
void Device::destroy(vkType object, vkTypes... objects) const
{
	destroy(object);
	destroy(objects...);
}
} // namespace le::gfx
