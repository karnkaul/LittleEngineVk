#pragma once
#include <core/ref.hpp>
#include <core/span.hpp>
#include <core/std_types.hpp>
#include <vulkan/vulkan.hpp>
#include <optional>

namespace le::graphics {
struct PhysicalDevice {
	vk::PhysicalDevice device;
	vk::PhysicalDeviceProperties properties;
	vk::PhysicalDeviceFeatures features;
	std::vector<vk::QueueFamilyProperties> queueFamilies;

	inline std::string_view name() const noexcept { return std::string_view(properties.deviceName); }
	inline bool discreteGPU() const noexcept { return properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu; }
	inline bool integratedGPU() const noexcept { return properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu; }
	inline bool virtualGPU() const noexcept { return properties.deviceType == vk::PhysicalDeviceType::eVirtualGpu; }

	bool surfaceSupport(u32 queueFamily, vk::SurfaceKHR surface) const;
	vk::SurfaceCapabilitiesKHR surfaceCapabilities(vk::SurfaceKHR surface) const;
	std::string toString() const;
};

std::ostream& operator<<(std::ostream& out, PhysicalDevice const& device);
} // namespace le::graphics
