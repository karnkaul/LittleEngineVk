#include <set>
#include "core/assert.hpp"
#include "core/utils.hpp"
#include "engine/vk/device.hpp"
#include "engine/vk/instanceImpl.hpp"

namespace le::vuk
{
std::string const Device::s_tName = utils::tName<Device>();

Device::Device(VkSurfaceKHR surface)
{
	m_surface = surface;
	auto pPhysicalDevice = g_instance.activeDevice();
	ASSERT(pPhysicalDevice, "Physical Device is null!");
	m_queueFamilyIndices.graphics = pPhysicalDevice->m_graphicsQueueFamilyIndex;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	for (size_t idx = 0; idx < pPhysicalDevice->m_queueFamilies.size(); ++idx)
	{
		if (pPhysicalDevice->m_physicalDevice.getSurfaceSupportKHR((u32)idx, surface))
		{
			m_queueFamilyIndices.presentation.emplace((u32)idx);
			std::set<u32> uniqueFamilies = {m_queueFamilyIndices.graphics.value(), (u32)idx};
			f32 priority = 1.0f;
			for (auto family : uniqueFamilies)
			{
				vk::DeviceQueueCreateInfo queueCreateInfo;
				queueCreateInfo.queueFamilyIndex = family;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &priority;
				queueCreateInfos.push_back(std::move(queueCreateInfo));
			}
		}
	}
	vk::PhysicalDeviceFeatures deviceFeatures;
	vk::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = (u32)queueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	if (!g_instance.m_layers.empty())
	{
		deviceCreateInfo.enabledLayerCount = (u32)g_instance.m_layers.size();
		deviceCreateInfo.ppEnabledLayerNames = g_instance.m_layers.data();
	}
	m_device = pPhysicalDevice->m_physicalDevice.createDevice(deviceCreateInfo);
	m_graphicsQueue = m_device.getQueue(m_queueFamilyIndices.graphics.value(), 0);
	m_presentationQueue = m_device.getQueue(m_queueFamilyIndices.presentation.value(), 0);
}

Device::~Device()
{
	if (m_surface != vk::SurfaceKHR())
	{
		vuk::g_instance.destroy(m_surface);
	}
	if (m_device != vk::Device())
	{
		m_device.destroy();
	}
}
} // namespace le
