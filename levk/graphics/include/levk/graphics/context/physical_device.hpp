#pragma once
#include <levk/graphics/common.hpp>

namespace le::graphics {
struct PhysicalDevice {
	vk::PhysicalDevice device;
	vk::PhysicalDeviceProperties properties;
	vk::PhysicalDeviceMemoryProperties memoryProperties;
	vk::PhysicalDeviceFeatures features;
	std::vector<vk::QueueFamilyProperties> queueFamilies;

	std::string_view name() const noexcept { return std::string_view(properties.deviceName); }
	bool discreteGPU() const noexcept { return properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu; }
	bool integratedGPU() const noexcept { return properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu; }
	bool virtualGPU() const noexcept { return properties.deviceType == vk::PhysicalDeviceType::eVirtualGpu; }
	bool supportsLazyAllocation() const noexcept;

	bool surfaceSupport(u32 queueFamily, vk::SurfaceKHR surface) const;
	vk::SurfaceCapabilitiesKHR surfaceCapabilities(vk::SurfaceKHR surface) const;
	vk::FormatProperties formatProperties(vk::Format format) const { return device.getFormatProperties(format); }
	BlitCaps blitCaps(vk::Format format) const;
	std::string toString() const;

  private:
	mutable std::unordered_map<vk::Format, BlitCaps> m_blitCaps;
};

std::ostream& operator<<(std::ostream& out, PhysicalDevice const& device);
} // namespace le::graphics
