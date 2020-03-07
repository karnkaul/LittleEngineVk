#include <set>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/vuk/instance/physical_device.hpp"

namespace le::vuk
{
std::string const PhysicalDevice::s_tName = utils::tName<PhysicalDevice>();
std::vector<char const*> const PhysicalDevice::s_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

PhysicalDevice::PhysicalDevice(vk::PhysicalDevice device)
{
	m_physicalDevice = std::move(device);
	m_properties = m_physicalDevice.getProperties();
	m_type = m_properties.deviceType;
	m_name = m_properties.deviceName;
	m_queueFamilies = m_physicalDevice.getQueueFamilyProperties();
	for (size_t idx = 0; idx < m_queueFamilies.size() && !m_graphicsQueueFamilyIndex.has_value(); ++idx)
	{
		auto const& queueFamily = m_queueFamilies.at(idx);
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			m_graphicsQueueFamilyIndex.emplace((u32)idx);
		}
	}
	std::set<std::string_view> requiredExtensions = {s_deviceExtensions.begin(), s_deviceExtensions.end()};
	auto const extensions = m_physicalDevice.enumerateDeviceExtensionProperties();
	for (auto const& extension : extensions)
	{
		requiredExtensions.erase(std::string_view(extension.extensionName));
	}
	for (auto extension : requiredExtensions)
	{
		LOG_E("[{}] Required extensions not present on physical device [{}]!", s_tName, extension);
	}
	ASSERT(requiredExtensions.empty(), "Required extension not present!");
	if (!requiredExtensions.empty())
	{
		throw std::runtime_error("Required extensions not present on physical device!");
	}
}

vk::Device PhysicalDevice::createDevice(vk::DeviceCreateInfo const& createInfo) const
{
	return m_physicalDevice.createDevice(createInfo);
}

PhysicalDevice::operator vk::PhysicalDevice const&() const
{
	return m_physicalDevice;
}
} // namespace le::vuk
