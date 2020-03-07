#pragma once
#include <optional>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "core/std_types.hpp"
#include "physical_device.hpp"
#include "engine/window/window_id.hpp"
#include "engine/vuk/context/swapchain_data.hpp"

namespace le::vuk
{
class Device final
{
public:
	struct QueueFamilyIndices final
	{
		std::optional<u32> graphics;
		std::optional<u32> present;

		constexpr bool isReady() const
		{
			return graphics.has_value() && present.has_value();
		}
	};

public:
	static std::string const s_tName;

public:
	std::string m_name;
	QueueFamilyIndices m_queueFamilyIndices;
	vk::PhysicalDeviceType m_type = vk::PhysicalDeviceType::eOther;

private:
	std::vector<PhysicalDevice> m_physicalDevices;
	PhysicalDevice* m_pPhysicalDevice = nullptr;
	vk::Device m_device;
	vk::Queue m_graphicsQueue;
	vk::Queue m_presentationQueue;

public:
	Device(vk::Instance const& instance, std::vector<char const*> const& layers, vk::SurfaceKHR surface);
	~Device();

public:
	explicit operator vk::Device const&() const;
	explicit operator vk::PhysicalDevice const&() const;
	std::unique_ptr<class Swapchain> createSwapchain(SwapchainData const& data, WindowID window) const;
	bool validateSurface(vk::SurfaceKHR const& surface) const;

	template <typename vkType>
	void destroy(vkType const& handle) const;

private:
	bool validateSurface(vk::SurfaceKHR const& surface, u32 presentFamilyIndex) const;
};

template <typename vkType>
void Device::destroy(vkType const& handle) const
{
	if (handle != vkType())
	{
		m_device.destroy(handle);
	}
	return;
}
} // namespace le::vuk
