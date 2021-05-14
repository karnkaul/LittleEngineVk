#pragma once
#include <core/ref.hpp>
#include <core/span.hpp>
#include <core/std_types.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
struct PhysicalDevice final {
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

	friend std::ostream& operator<<(std::ostream& out, PhysicalDevice const& device) {
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
};

class DevicePicker {
  public:
	using Score = s32;

	inline static constexpr Score discrete = 100;
	inline static constexpr Score integrated = -20;

	inline static void addIf(Score& out_base, bool predicate, Score mod) noexcept {
		if (predicate) {
			out_base += mod;
		}
	}

	PhysicalDevice pick(View<PhysicalDevice> devices, std::optional<std::size_t> indexOverride) const;
	Score score(PhysicalDevice const& device) const;

	///
	/// \brief Override to modify the base score of a device
	/// (Returns base score by default)
	///
	virtual inline Score modify(Score base, PhysicalDevice const&) const { return base; }
	///
	/// \brief Override to select a device from a list with identical scores
	/// (Returns front element by default)
	///
	virtual PhysicalDevice tieBreak(View<Ref<PhysicalDevice const>> devices) const {
		ENSURE(!devices.empty(), "Empty list");
		return devices.begin()->get();
	}
};
} // namespace le::graphics
