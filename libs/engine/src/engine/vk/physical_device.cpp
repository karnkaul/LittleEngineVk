#include <set>
#include "core/utils.hpp"
#include "engine/vk/physical_device.hpp"

namespace le::vuk
{
std::string const PhysicalDevice::s_tName = utils::tName<PhysicalDevice>();

PhysicalDevice::PhysicalDevice(vk::PhysicalDevice device)
{
	m_physicalDevice = std::move(device);
	auto const properties = m_physicalDevice.getProperties();
	m_type = properties.deviceType;
	m_name = properties.deviceName;
	m_queueFamilies = m_physicalDevice.getQueueFamilyProperties();
	for (size_t idx = 0; idx < m_queueFamilies.size() && !m_graphicsQueueFamilyIndex.has_value(); ++idx)
	{
		auto const& queueFamily = m_queueFamilies.at(idx);
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			m_graphicsQueueFamilyIndex.emplace((u32)idx);
		}
	}
}
} // namespace le
