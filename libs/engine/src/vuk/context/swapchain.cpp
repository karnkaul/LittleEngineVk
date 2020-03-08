#include <array>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/window/window.hpp"
#include "engine/vuk/instance/device.hpp"
#include "engine/vuk/context/swapchain.hpp"
#include "vuk/instance/instance_impl.hpp"

namespace le::vuk
{
bool Swapchain::Details::isReady() const
{
	return !formats.empty() && !presentModes.empty();
}

vk::SurfaceFormatKHR Swapchain::Details::bestFormat() const
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

vk::PresentModeKHR Swapchain::Details::bestPresentMode() const
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

vk::Extent2D Swapchain::Details::extent(glm::ivec2 const& framebufferSize) const
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		return {(u32)framebufferSize.x, (u32)framebufferSize.y};
	}
}

std::string const Swapchain::s_tName = utils::tName<Swapchain>();

Swapchain::Swapchain(SwapchainData const& data, WindowID window) : m_window(window)
{
	recreate(data);
	LOG_D("[{}:{}] constructed", s_tName, m_window);
}

Swapchain::~Swapchain()
{
	destroy();
	LOG_D("[{}:{}] destroyed", s_tName, m_window);
}

void Swapchain::recreate(SwapchainData const& data)
{
	destroy();
	ASSERT(g_pDevice, "Device is null!");
	getDetails(data.surface);
	createSwapchain(data);
	populateImages();
	createRenderPasses();
	createFramebuffers();
	LOG_D("[{}:{}] created; framebuffer size: [{}x{}]", s_tName, m_window, data.framebufferSize.x, data.framebufferSize.y);
	return;
}

void Swapchain::destroy()
{
	ASSERT(g_pDevice, "Device is null!");
	LOGIF_D(m_swapchain != vk::SwapchainKHR(), "[{}:{}] released", s_tName, m_window);
	for (auto const& frameBuffer : m_framebuffers)
	{
		g_pDevice->destroy(frameBuffer);
	}
	g_pDevice->destroy(m_defaultRenderPass);
	for (auto const& imageView : m_imageViews)
	{
		g_pDevice->destroy(imageView);
	}
	if (m_swapchain != vk::SwapchainKHR())
	{
		g_pDevice->destroy(m_swapchain);
	}
	m_framebuffers.clear();
	m_defaultRenderPass = vk::RenderPass();
	m_images.clear();
	m_imageViews.clear();
	m_swapchain = vk::SwapchainKHR();
	m_details = {};
	return;
}

Swapchain::operator vk::SwapchainKHR const&() const
{
	return m_swapchain;
}

void Swapchain::getDetails(vk::SurfaceKHR const& surface)
{
	auto vkPhysicalDevice = static_cast<vk::PhysicalDevice const&>(*g_pDevice);
	m_details = {
		vkPhysicalDevice.getSurfaceCapabilitiesKHR(surface),
		vkPhysicalDevice.getSurfaceFormatsKHR(surface),
		vkPhysicalDevice.getSurfacePresentModesKHR(surface),
	};
	if (!m_details.isReady())
	{
		throw std::runtime_error("Failed to get Swapchain details!");
	}
	return;
}

void Swapchain::createSwapchain(SwapchainData const& data)
{
	auto const& queueFamilyIndices = g_pDevice->m_queueFamilyIndices;
	auto const& vkDevice = static_cast<vk::Device const&>(*g_pDevice);
	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.minImageCount = m_details.capabilities.minImageCount + 1;
	if (m_details.capabilities.maxImageCount != 0 && createInfo.minImageCount > m_details.capabilities.maxImageCount)
	{
		createInfo.minImageCount = m_details.capabilities.maxImageCount;
	}
	auto const format = m_details.bestFormat();
	m_format = format.format;
	createInfo.imageFormat = m_format;
	createInfo.imageColorSpace = format.colorSpace;
	m_extent = m_details.extent(data.framebufferSize);
	createInfo.imageExtent = m_extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	u32 const graphicsFamilyIndex = queueFamilyIndices.graphics.value();
	std::array const indices = {graphicsFamilyIndex, queueFamilyIndices.present.value()};
	if (indices.at(0) != indices.at(1))
	{
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = (u32)indices.size();
		createInfo.pQueueFamilyIndices = indices.data();
	}
	else
	{
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 1;
		createInfo.pQueueFamilyIndices = &graphicsFamilyIndex;
	}
	createInfo.preTransform = m_details.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = m_details.bestPresentMode();
	createInfo.clipped = vk::Bool32(true);
	createInfo.surface = data.surface;
	m_swapchain = vkDevice.createSwapchainKHR(createInfo);
	return;
}

void Swapchain::populateImages()
{
	auto const vkDevice = static_cast<vk::Device const&>(*g_pDevice);
	m_images = vkDevice.getSwapchainImagesKHR(m_swapchain);
	m_imageViews.reserve(m_images.size());
	for (auto const& image : m_images)
	{
		vk::ImageViewCreateInfo createInfo;
		createInfo.image = image;
		createInfo.viewType = vk::ImageViewType::e2D;
		createInfo.format = m_format;
		createInfo.components.r = createInfo.components.g = createInfo.components.b = createInfo.components.a =
			vk::ComponentSwizzle::eIdentity;
		createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		m_imageViews.push_back(vkDevice.createImageView(createInfo));
	}
	if (m_images.empty() || m_imageViews.empty())
	{
		throw std::runtime_error("Failed to populate Swapchain images!");
	}
	return;
}

void Swapchain::createRenderPasses()
{
	auto const vkDevice = static_cast<vk::Device const&>(*g_pDevice);
	vk::AttachmentDescription colourDesc;
	colourDesc.format = m_format;
	colourDesc.samples = vk::SampleCountFlagBits::e1;
	colourDesc.loadOp = vk::AttachmentLoadOp::eClear;
	colourDesc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colourDesc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colourDesc.initialLayout = vk::ImageLayout::eUndefined;
	colourDesc.finalLayout = vk::ImageLayout::ePresentSrcKHR;
	vk::AttachmentReference colourRef;
	colourRef.attachment = 0;
	colourRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourRef;
	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colourDesc;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	m_defaultRenderPass = vkDevice.createRenderPass(createInfo);
	return;
}

void Swapchain::createFramebuffers()
{
	auto const vkDevice = static_cast<vk::Device const&>(*g_pDevice);
	m_framebuffers.reserve(m_imageViews.size());
	for (auto const& imageView : m_imageViews)
	{
		std::array attachments = {imageView};
		vk::FramebufferCreateInfo createInfo;
		createInfo.attachmentCount = (u32)attachments.size();
		createInfo.pAttachments = attachments.data();
		createInfo.renderPass = m_defaultRenderPass;
		createInfo.width = m_extent.width;
		createInfo.height = m_extent.height;
		createInfo.layers = 1;
		auto frameBuffer = vkDevice.createFramebuffer(createInfo);
		m_framebuffers.push_back(std::move(frameBuffer));
	}
	return;
}
} // namespace le::vuk
