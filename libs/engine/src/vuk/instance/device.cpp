#include <set>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/vuk/instance/device.hpp"
#include "engine/vuk/context/swapchain.hpp"
#include "vuk/instance/instance_impl.hpp"

namespace le::vuk
{
std::string const Device::s_tName = utils::tName<Device>();

Device::Device(vk::Instance const& instance, std::vector<char const*> const& layers, vk::SurfaceKHR surface)
{
	auto devices = vk::Instance(instance).enumeratePhysicalDevices();
	m_physicalDevices.reserve(devices.size());
	for (auto const& device : devices)
	{
		m_physicalDevices.emplace_back(device);
		if (m_physicalDevices.back().m_type == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			m_pPhysicalDevice = &m_physicalDevices.back();
		}
	}
	if (!m_pPhysicalDevice)
	{
		LOG_E("[{}] Failed to select a [{}]!", s_tName, PhysicalDevice::s_tName);
		throw std::runtime_error("Failed to select a physical device!");
	}
	auto const properties = m_pPhysicalDevice->m_properties;
	m_type = properties.deviceType;
	m_name = properties.deviceName;
	auto const queueFamilies = m_pPhysicalDevice->m_queueFamilies;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	for (size_t idx = 0; idx < queueFamilies.size() && !m_queueFamilyIndices.isReady(); ++idx)
	{
		auto const& queueFamily = queueFamilies.at(idx);
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			m_queueFamilyIndices.graphics.emplace((u32)idx);
		}
		if (validateSurface(surface, (u32)idx))
		{
			m_queueFamilyIndices.present.emplace((u32)idx);
		}
	}
	std::set<u32> uniqueFamilies = {m_queueFamilyIndices.graphics.value(), (u32)m_queueFamilyIndices.present.value()};
	f32 priority = 1.0f;
	for (auto family : uniqueFamilies)
	{
		vk::DeviceQueueCreateInfo queueCreateInfo;
		queueCreateInfo.queueFamilyIndex = family;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &priority;
		queueCreateInfos.push_back(std::move(queueCreateInfo));
	}
	vk::PhysicalDeviceFeatures deviceFeatures;
	vk::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = (u32)queueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	if (!layers.empty())
	{
		deviceCreateInfo.enabledLayerCount = (u32)layers.size();
		deviceCreateInfo.ppEnabledLayerNames = layers.data();
	}
	if (!PhysicalDevice::s_deviceExtensions.empty())
	{
		deviceCreateInfo.enabledExtensionCount = (u32)PhysicalDevice::s_deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = PhysicalDevice::s_deviceExtensions.data();
	}
	m_device = m_pPhysicalDevice->createDevice(deviceCreateInfo);
	m_graphicsQueue = m_device.getQueue(m_queueFamilyIndices.graphics.value(), 0);
	m_presentationQueue = m_device.getQueue(m_queueFamilyIndices.present.value(), 0);
	LOG_I("[{}] constructed. Using GPU: [{}]", s_tName, m_pPhysicalDevice->m_name);
}

Device::~Device()
{
	if (m_device != vk::Device())
	{
		m_device.destroy();
		LOG_I("[{}] destroyed", s_tName);
	}
	return;
}

Device::operator vk::Device const&() const
{
	return m_device;
}

Device::operator vk::PhysicalDevice const&() const
{
	return static_cast<vk::PhysicalDevice const&>(*m_pPhysicalDevice);
}

std::unique_ptr<Swapchain> Device::createSwapchain(SwapchainData const& data, WindowID window) const
{
	if (!validateSurface(data.surface))
	{
		LOG_E("[{}] cannot present to passed [{}]!", s_tName, utils::tName<vk::SurfaceKHR>());
		return {};
	}
	return std::make_unique<Swapchain>(data, window);
}

bool Device::validateSurface(vk::SurfaceKHR const& surface) const
{
	return validateSurface(surface, m_queueFamilyIndices.present.value());
}

bool Device::validateSurface(vk::SurfaceKHR const& surface, u32 presentFamilyIndex) const
{
	return static_cast<vk::PhysicalDevice const&>(*m_pPhysicalDevice).getSurfaceSupportKHR(presentFamilyIndex, surface);
}
} // namespace le::vuk
