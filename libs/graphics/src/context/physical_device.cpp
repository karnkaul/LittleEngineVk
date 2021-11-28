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

BlitCaps PhysicalDevice::blitCaps(vk::Format format) const {
	using vFFFB = vk::FormatFeatureFlagBits;
	auto it = m_blitCaps.find(format);
	if (it == m_blitCaps.end()) {
		auto [i, _] = m_blitCaps.emplace(format, BlitCaps());
		it = i;
		auto const fp = formatProperties(format);
		if ((fp.linearTilingFeatures & vFFFB::eTransferSrc) == vFFFB::eTransferSrc) { it->second.linear.set(BlitFlag::eSrc); }
		if ((fp.linearTilingFeatures & vFFFB::eTransferDst) == vFFFB::eTransferDst) { it->second.linear.set(BlitFlag::eDst); }
		if ((fp.optimalTilingFeatures & vFFFB::eTransferSrc) == vFFFB::eTransferSrc) { it->second.optimal.set(BlitFlag::eSrc); }
		if ((fp.optimalTilingFeatures & vFFFB::eTransferDst) == vFFFB::eTransferDst) { it->second.optimal.set(BlitFlag::eDst); }
	}
	return it->second;
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
