#include <map>
#include <sstream>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/physical_device.hpp>

namespace le::graphics {
bool PhysicalDevice::surfaceSupport(u32 queueFamily, vk::SurfaceKHR surface) const {
	return !Device::default_v(device) && device.getSurfaceSupportKHR(queueFamily, surface);
}

vk::SurfaceCapabilitiesKHR PhysicalDevice::surfaceCapabilities(vk::SurfaceKHR surface) const {
	return !Device::default_v(device) ? device.getSurfaceCapabilitiesKHR(surface) : vk::SurfaceCapabilitiesKHR();
}

std::string PhysicalDevice::toString() const {
	std::stringstream str;
	str << *this;
	return str.str();
}

PhysicalDevice DevicePicker::pick(View<PhysicalDevice> devices, std::optional<std::size_t> indexOverride) const {
	ENSURE(!devices.empty(), "No devices to pick from");
	if (indexOverride && *indexOverride < devices.size()) {
		auto const& ret = devices[*indexOverride];
		g_log.log(lvl::info, 0, "[{}] Device selection overridden: [{}] {}", g_name, *indexOverride, ret.toString());
		return ret;
	}
	using DevList = kt::fixed_vector<Ref<PhysicalDevice const>, 4>;
	std::map<Score, DevList, std::greater<Score>> deviceMap;
	for (auto const& device : devices) {
		deviceMap[score(device)].emplace_back(device);
	}
	DevList const& list = deviceMap.begin()->second;
	return list.size() == 1 ? list.front().get() : tieBreak(deviceMap.begin()->second);
}

DevicePicker::Score DevicePicker::score(PhysicalDevice const& device) const {
	Score total = 0;
	addIf(total, device.discreteGPU(), discrete);
	addIf(total, device.integratedGPU(), integrated);
	return modify(total, device);
}
} // namespace le::graphics
