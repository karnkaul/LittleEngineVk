#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/physical_device.hpp>
#include <sstream>

namespace le::graphics {
bool PhysicalDevice::surfaceSupport(u32 queueFamily, vk::SurfaceKHR surface) const {
	return !Device::default_v(device) && device.getSurfaceSupportKHR(queueFamily, surface);
}

bool PhysicalDevice::supportsLazyAllocation() const noexcept {
	for (u32 i = 0; i < memoryProperties.memoryTypeCount; ++i) {
		if ((memoryProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eLazilyAllocated) == vk::MemoryPropertyFlagBits::eLazilyAllocated) {
			return true;
		}
	}
	return false;
}

vk::SurfaceCapabilitiesKHR PhysicalDevice::surfaceCapabilities(vk::SurfaceKHR surface) const {
	return !Device::default_v(device) ? device.getSurfaceCapabilitiesKHR(surface) : vk::SurfaceCapabilitiesKHR();
}

std::ostream& operator<<(std::ostream& out, PhysicalDevice const& device) {
	static constexpr std::string_view discrete = " (Discrete)";
	static constexpr std::string_view integrated = " (Integrated)";
	out << device.name();
	if (device.discreteGPU()) {
		out << discrete;
	} else if (device.integratedGPU()) {
		out << integrated;
	}
	return out;
}

std::string PhysicalDevice::toString() const {
	std::stringstream str;
	str << *this;
	return str.str();
}
} // namespace le::graphics
