#include <array>
#include <limits>
#include <map>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/window/window.hpp"
#include "swapchain.hpp"
#include "vuk/info.hpp"

namespace le::vuk
{
bool Swapchain::Details::isReady() const
{
	return !formats.empty() && !presentModes.empty();
}

vk::SurfaceFormatKHR Swapchain::Details::bestFormat(SwapchainData::Options options) const
{
	if (!options.format.has_value())
	{
		options.format.emplace(vk::Format::eB8G8R8A8Srgb);
	}
	if (!options.colourSpace.has_value())
	{
		options.colourSpace.emplace(vk::ColorSpaceKHR::eSrgbNonlinear);
	}
	std::map<u8, vk::SurfaceFormatKHR> availableFormats;
	for (auto const& format : formats)
	{
		u8 rank = 0xff;
		if (format.format == options.format.value())
		{
			--rank;
		}
		if (format.colorSpace == options.colourSpace.value())
		{
			--rank;
		}
		availableFormats.emplace(rank, format);
	}
	return availableFormats.empty() ? vk::SurfaceFormatKHR() : availableFormats.begin()->second;
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

Swapchain::Swapchain(SwapchainData const& data)
	: m_getNewSurface(data.config.createNewSurface),
	  m_options(data.options),
	  m_framebufferSize(data.config.framebufferSize),
	  m_window(data.config.window)
{
	ASSERT(m_getNewSurface, "Surface generator is null!");
	create();
	LOG_D("[{}:{}] constructed", s_tName, m_window);
}

Swapchain::~Swapchain()
{
	release();
	vkDestroy<vk::Instance>(m_surface);
	LOG_D("[{}:{}] destroyed", s_tName, m_window);
}

void Swapchain::recreate(glm::ivec2 const& framebufferSize)
{
	m_framebufferSize = framebufferSize;
	release();
	create();
}

vk::RenderPassBeginInfo Swapchain::acquireNextImage(vk::Semaphore wait, vk::Fence setInUse)
{
	[[maybe_unused]] auto result = g_info.device.acquireNextImageKHR(m_swapchain, maxVal<u64>(), wait, {});
	LOGIF_D(result.result != vk::Result::eSuccess, "acquireNextImageKHR result: {}", result.result);
	size_t const idx = (size_t)result.value;
	if (m_inUseFences.at(idx) != vk::Fence())
	{
		g_info.device.waitForFences(m_inUseFences.at(idx), true, maxVal<u64>());
	}
	m_inUseFences.at(idx) = setInUse;
	m_currentIndex = (u32)idx;
	vk::RenderPassBeginInfo ret;
	ret.renderPass = m_defaultRenderPass;
	ret.framebuffer = m_framebuffers.at(idx);
	ret.renderArea.extent = m_extent;
	return ret;
}

vk::Result Swapchain::present(vk::Semaphore wait) const
{
	vk::PresentInfoKHR presentInfo;
	auto swapchain = static_cast<vk::SwapchainKHR>(*this);
	auto const index = m_currentIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &wait;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &index;
	auto result = g_info.queues.present.presentKHR(&presentInfo);
	LOGIF_D(result != vk::Result::eSuccess, "presentKHR result: {}", result);
	return result;
}

Swapchain::operator vk::SwapchainKHR() const
{
	return m_swapchain;
}

void Swapchain::getDetails()
{
	m_details = {
		g_info.physicalDevice.getSurfaceCapabilitiesKHR(m_surface),
		g_info.physicalDevice.getSurfaceFormatsKHR(m_surface),
		g_info.physicalDevice.getSurfacePresentModesKHR(m_surface),
	};
	if (!m_details.isReady())
	{
		throw std::runtime_error("Failed to get Swapchain details!");
	}
	return;
}

void Swapchain::create()
{
	auto oldSurface = m_surface;
	if (m_surface == vk::SurfaceKHR() || !g_info.isValid(m_surface))
	{
		m_surface = m_getNewSurface(g_info.instance);
	}
	[[maybe_unused]] bool bValid = g_info.isValid(m_surface);
	ASSERT(bValid, "Invalid surface!");
	getDetails();
	createSwapchain();
	populateImages();
	createRenderPasses();
	createFramebuffers();
	m_inUseFences.resize(m_images.size());
	if (oldSurface != m_surface)
	{
		vkDestroy<vk::Instance>(oldSurface);
	}
	LOG_D("[{}:{}] created; framebuffer size: [{}x{}]", s_tName, m_window, m_framebufferSize.x, m_framebufferSize.y);
	return;
}

void Swapchain::createSwapchain()
{
	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.minImageCount = m_details.capabilities.minImageCount + 1;
	if (m_details.capabilities.maxImageCount != 0 && createInfo.minImageCount > m_details.capabilities.maxImageCount)
	{
		createInfo.minImageCount = m_details.capabilities.maxImageCount;
	}
	auto const format = m_details.bestFormat(m_options);
	m_format = format.format;
	createInfo.imageFormat = m_format;
	createInfo.imageColorSpace = format.colorSpace;
	m_extent = m_details.extent(m_framebufferSize);
	createInfo.imageExtent = m_extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	std::array const indices = {g_info.queueFamilyIndices.graphics, g_info.queueFamilyIndices.present};
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
		createInfo.pQueueFamilyIndices = &g_info.queueFamilyIndices.graphics;
	}
	createInfo.preTransform = m_details.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = m_details.bestPresentMode();
	createInfo.clipped = vk::Bool32(true);
	createInfo.surface = m_surface;
	m_swapchain = g_info.device.createSwapchainKHR(createInfo);
	return;
}

void Swapchain::populateImages()
{
	m_images = g_info.device.getSwapchainImagesKHR(m_swapchain);
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
		m_imageViews.push_back(g_info.device.createImageView(createInfo));
	}
	if (m_images.empty() || m_imageViews.empty())
	{
		throw std::runtime_error("Failed to populate Swapchain images!");
	}
	return;
}

void Swapchain::createRenderPasses()
{
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
	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;
	m_defaultRenderPass = g_info.device.createRenderPass(createInfo);
	return;
}

void Swapchain::createFramebuffers()
{
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
		auto frameBuffer = g_info.device.createFramebuffer(createInfo);
		m_framebuffers.push_back(std::move(frameBuffer));
	}
	return;
}

void Swapchain::release()
{
	g_info.device.waitIdle();
	for (auto const& frameBuffer : m_framebuffers)
	{
		vkDestroy(frameBuffer);
	}
	vkDestroy(m_defaultRenderPass);
	for (auto const& imageView : m_imageViews)
	{
		vkDestroy(imageView);
	}
	vkDestroy(m_swapchain);
	m_framebuffers.clear();
	m_defaultRenderPass = vk::RenderPass();
	m_images.clear();
	m_imageViews.clear();
	m_inUseFences.clear();
	LOGIF_D(m_swapchain != vk::SwapchainKHR(), "[{}:{}] released", s_tName, m_window);
	m_swapchain = vk::SwapchainKHR();
	m_details = {};
	return;
}
} // namespace le::vuk
