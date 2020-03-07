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
	static std::vector<char const*> const s_deviceExtensions;

public:
	vk::PhysicalDeviceProperties m_properties;
	vk::PhysicalDeviceType m_type = vk::PhysicalDeviceType::eOther;
	std::vector<vk::QueueFamilyProperties> m_queueFamilies;
	std::optional<u32> m_graphicsQueueFamilyIndex;
	std::string m_name;

private:
	vk::PhysicalDevice m_physicalDevice;

public:
	PhysicalDevice(vk::PhysicalDevice device);

public:
	vk::Device createDevice(vk::DeviceCreateInfo const& createInfo) const;

public:
	explicit operator vk::PhysicalDevice const&() const;

private:
	friend class Device;
};
} // namespace le::vuk
