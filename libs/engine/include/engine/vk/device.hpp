#pragma once
#include <optional>
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"

namespace le
{
class VkDevice final
{
public:
	struct QueueFamilyIndices
	{
		std::optional<u32> graphics;
		std::optional<u32> presentation;

		constexpr bool isReady()
		{
			return graphics.has_value() /* && presentation.has_value() */;
		}
	};

public:
	static std::string const s_tName;

public:
	std::string m_name;
	QueueFamilyIndices m_queueFamilyIndices;
	vk::PhysicalDeviceType m_type = vk::PhysicalDeviceType::eOther;

private:
	vk::PhysicalDevice m_physicalDevice;
	vk::Device m_device;

public:
	VkDevice(vk::PhysicalDevice device, std::vector<char const*> const& validationLayers);
	~VkDevice();
};
} // namespace le
