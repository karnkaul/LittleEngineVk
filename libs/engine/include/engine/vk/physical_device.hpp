#pragma once
#include <optional>
#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"

namespace le::vuk
{
class PhysicalDevice final
{
public:
	static std::string const s_tName;

public:
	std::string m_name;
	vk::PhysicalDeviceType m_type = vk::PhysicalDeviceType::eOther;
	std::optional<u32> m_graphicsQueueFamilyIndex;

private:
	vk::PhysicalDevice m_physicalDevice;
	std::vector<vk::QueueFamilyProperties> m_queueFamilies;

public:
	PhysicalDevice(vk::PhysicalDevice device);

private:
	friend class Device;
};
} // namespace le::vuk
