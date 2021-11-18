#include <core/utils/error.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/physical_device.hpp>
#include <map>
#include <sstream>

namespace le::graphics {
bool PhysicalDevice::surfaceSupport(u32 queueFamily, vk::SurfaceKHR surface) const {
	return !Device::default_v(device) && device.getSurfaceSupportKHR(queueFamily, surface);
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

PhysicalDevice DevicePicker::pick(Span<PhysicalDevice const> devices, std::optional<std::size_t> indexOverride) const {
	ENSURE(!devices.empty(), "No devices to pick from");
	if (indexOverride && *indexOverride < devices.size()) {
		auto const& ret = devices[*indexOverride];
		g_log.log(lvl::info, 0, "[{}] Device selection overridden: [{}] {}", g_name, *indexOverride, ret.toString());
		return ret;
	}
	using DevList = ktl::fixed_vector<Ref<PhysicalDevice const>, 4>;
	std::map<Score, DevList, std::greater<Score>> deviceMap;
	for (auto const& device : devices) { deviceMap[score(device)].emplace_back(device); }
	DevList const& list = deviceMap.begin()->second;
	return list.size() == 1 ? list.front().get() : tieBreak(deviceMap.begin()->second);
}

DevicePicker::Score DevicePicker::score(PhysicalDevice const& device) const {
	Score total = 0;
	addIf(total, device.discreteGPU(), discrete);
	addIf(total, device.integratedGPU(), integrated);
	return modify(total, device);
}

PhysicalDevice DevicePicker::tieBreak(Span<Ref<PhysicalDevice const> const> devices) const {
	ENSURE(!devices.empty(), "Empty list");
	return devices.begin()->get();
}
} // namespace le::graphics
