#include <array>
#include <set>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/vk/device.hpp"
#include "engine/vk/physical_device.hpp"
#include "engine/vk/instanceImpl.hpp"

namespace le::vuk
{
bool Device::SwapchainDetails::isReady() const
{
	return !formats.empty() && !presentModes.empty();
}

vk::SurfaceFormatKHR Device::SwapchainDetails::bestFormat() const
{
	for (auto const& format : formats)
	{
		if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			return format;
		}
	}
	return formats.empty() ? vk::SurfaceFormatKHR() : formats.front();
}

vk::PresentModeKHR Device::SwapchainDetails::bestPresentMode() const
{
	for (auto const& presentMode : presentModes)
	{
		if (presentMode == vk::PresentModeKHR::eMailbox)
		{
			return presentMode;
		}
	}
	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Device::SwapchainDetails::extent(glm::ivec2 const& windowSize) const
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D ret{std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, (u32)windowSize.x)),
					   std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, (u32)windowSize.y))};
		return ret;
	}
}

std::string const Device::s_tName = utils::tName<Device>();

Device::Device(Data&& data)
{
	m_surface = std::move(data.surface);
	auto pPhysicalDevice = g_instance.activeDevice();
	ASSERT(pPhysicalDevice, "Physical Device is null!");
	auto& physicalDevice = pPhysicalDevice->m_physicalDevice;
	m_queueFamilyIndices.graphics = pPhysicalDevice->m_graphicsQueueFamilyIndex;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	for (size_t idx = 0; idx < pPhysicalDevice->m_queueFamilies.size(); ++idx)
	{
		if (physicalDevice.getSurfaceSupportKHR((u32)idx, m_surface))
		{
			m_queueFamilyIndices.present.emplace((u32)idx);
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
	if (!PhysicalDevice::s_deviceExtensions.empty())
	{
		deviceCreateInfo.enabledExtensionCount = (u32)PhysicalDevice::s_deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = PhysicalDevice::s_deviceExtensions.data();
	}
	m_device = physicalDevice.createDevice(deviceCreateInfo);
	m_graphicsQueue = m_device.getQueue(m_queueFamilyIndices.graphics.value(), 0);
	m_presentationQueue = m_device.getQueue(m_queueFamilyIndices.present.value(), 0);
	createSwapchain(physicalDevice, data.windowSize);
}

Device::~Device()
{
	for (auto const& imageView : m_swapchainImageViews)
	{
		m_device.destroy(imageView);
	}
	if (m_swapchain != vk::SwapchainKHR())
	{
		m_device.destroy(m_swapchain);
	}
	if (m_surface != vk::SurfaceKHR())
	{
		vuk::g_instance.destroy(m_surface);
	}
	if (m_device != vk::Device())
	{
		m_device.destroy();
	}
}

bool Device::createSwapchain(vk::PhysicalDevice const& physicalDevice, glm::ivec2 const& windowSize)
{
	// Swapchain Details
	m_swapchainDetails = {
		physicalDevice.getSurfaceCapabilitiesKHR(m_surface),
		physicalDevice.getSurfaceFormatsKHR(m_surface),
		physicalDevice.getSurfacePresentModesKHR(m_surface),
	};
	ASSERT(m_swapchainDetails.isReady(), "Unsupported swapchain!");
	if (!m_swapchainDetails.isReady())
	{
		LOG_E("[{}] No valid swapchain formats/present modes!", s_tName);
		return false;
	}

	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.minImageCount = m_swapchainDetails.capabilities.minImageCount + 1;
	if (m_swapchainDetails.capabilities.maxImageCount != 0 && createInfo.minImageCount > m_swapchainDetails.capabilities.maxImageCount)
	{
		createInfo.minImageCount = m_swapchainDetails.capabilities.maxImageCount;
	}
	auto const format = m_swapchainDetails.bestFormat();
	m_swapchainFormat = format.format;
	createInfo.imageFormat = m_swapchainFormat;
	createInfo.imageColorSpace = format.colorSpace;
	m_swapchainExtent = m_swapchainDetails.extent(windowSize);
	createInfo.imageExtent = m_swapchainExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	u32 const graphicsFamilyIndex = m_queueFamilyIndices.graphics.value();
	std::array const queueFamilyIndices = {graphicsFamilyIndex, m_queueFamilyIndices.present.value()};
	if (queueFamilyIndices.at(0) != queueFamilyIndices.at(1))
	{
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = (u32)queueFamilyIndices.size();
		createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	}
	else
	{
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 1;
		createInfo.pQueueFamilyIndices = &graphicsFamilyIndex;
	}
	createInfo.preTransform = m_swapchainDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = m_swapchainDetails.bestPresentMode();
	createInfo.clipped = vk::Bool32(true);
	createInfo.surface = m_surface;
	m_swapchain = m_device.createSwapchainKHR(createInfo);
	if (m_swapchain == vk::SwapchainKHR())
	{
		LOG_E("[{}] Failed to create swapchain!", s_tName);
		return false;
	}
	m_swapchainImages = m_device.getSwapchainImagesKHR(m_swapchain);
	m_swapchainImageViews.reserve(m_swapchainImages.size());
	for (auto const& image : m_swapchainImages)
	{
		vk::ImageViewCreateInfo createInfo;
		createInfo.image = image;
		createInfo.viewType = vk::ImageViewType::e2D;
		createInfo.format = m_swapchainFormat;
		createInfo.components.r = createInfo.components.g = createInfo.components.b = createInfo.components.a =
			vk::ComponentSwizzle::eIdentity;
		createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		m_swapchainImageViews.push_back(m_device.createImageView(createInfo));
	}
	return true;
}
} // namespace le::vuk
