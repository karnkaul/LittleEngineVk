#include <levk/graphics/common.hpp>
#include <levk/graphics/context/device.hpp>
#include <levk/graphics/context/physical_device.hpp>
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
		BlitCaps bc;
		auto const fp = formatProperties(format);
		if (fp.linearTilingFeatures & vFFFB::eBlitSrc) { bc.linear.set(BlitFlag::eSrc); }
		if (fp.linearTilingFeatures & vFFFB::eBlitDst) { bc.linear.set(BlitFlag::eDst); }
		if (fp.linearTilingFeatures & vFFFB::eSampledImageFilterLinear) { bc.linear.set(BlitFlag::eLinearFilter); }
		if (fp.optimalTilingFeatures & vFFFB::eBlitSrc) { bc.optimal.set(BlitFlag::eSrc); }
		if (fp.optimalTilingFeatures & vFFFB::eBlitDst) { bc.optimal.set(BlitFlag::eDst); }
		if (fp.optimalTilingFeatures & vFFFB::eSampledImageFilterLinear) { bc.optimal.set(BlitFlag::eLinearFilter); }
		auto [i, _] = m_blitCaps.emplace(format, bc);
		it = i;
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
