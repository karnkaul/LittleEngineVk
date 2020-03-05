#include "core/utils.hpp"
#include "engine/vk/device.hpp"

namespace le
{
std::string const VkDevice::s_tName = utils::tName<VkDevice>();

VkDevice::VkDevice(vk::PhysicalDevice device, std::vector<char const*> const& validationLayers)
{
	m_physicalDevice = std::move(device);
	auto const properties = m_physicalDevice.getProperties();
	m_type = properties.deviceType;
	m_name = properties.deviceName;
	auto const queueFamilyProperties = m_physicalDevice.getQueueFamilyProperties();
	for (size_t idx = 0; idx < queueFamilyProperties.size() && !m_queueFamilyIndices.isReady(); ++idx)
	{
		auto const& queueFamily = queueFamilyProperties.at(idx);
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			m_queueFamilyIndices.graphics.emplace((u32)idx);
		}
	}

	f32 const queuePriority = 1.0f;
	vk::DeviceQueueCreateInfo queueCreateInfo;
	queueCreateInfo.queueFamilyIndex = m_queueFamilyIndices.graphics.value();
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;
	vk::PhysicalDeviceFeatures deviceFeatures;
	vk::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	if (!validationLayers.empty())
	{
		deviceCreateInfo.enabledLayerCount = (u32)validationLayers.size();
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	m_device = m_physicalDevice.createDevice(deviceCreateInfo);
}

VkDevice::~VkDevice()
{
	if (m_device != vk::Device())
	{
		m_device.destroy();
	}
}
} // namespace le
