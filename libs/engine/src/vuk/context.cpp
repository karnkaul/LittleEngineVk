#include <map>
#include "core/log.hpp"
#include "core/utils.hpp"
#include "context.hpp"
#include "info.hpp"
#include "rendering.hpp"

namespace le::vuk
{
std::string const Context::s_tName = utils::tName<Context>();

void Context::Info::refresh()
{
	if (surface == vk::SurfaceKHR() || !g_info.isValid(surface))
	{
		surface = data.config.getNewSurface(g_info.instance);
	}
	[[maybe_unused]] bool bValid = g_info.isValid(surface);
	ASSERT(bValid, "Invalid surface!");
	capabilities = g_info.physicalDevice.getSurfaceCapabilitiesKHR(surface);
	formats = g_info.physicalDevice.getSurfaceFormatsKHR(surface);
	presentModes = g_info.physicalDevice.getSurfacePresentModesKHR(surface);
}

bool Context::Info::isReady() const
{
	return !formats.empty() && !presentModes.empty();
}

vk::SurfaceFormatKHR Context::Info::bestColourFormat() const
{
	vk::Format desiredFormat = data.options.format.has_value() ? data.options.format.value() : vk::Format::eB8G8R8A8Srgb;
	vk::ColorSpaceKHR desiredColourSpace =
		data.options.colourSpace.has_value() ? data.options.colourSpace.value() : vk::ColorSpaceKHR::eSrgbNonlinear;
	std::map<u8, vk::SurfaceFormatKHR> availableFormats;
	for (auto const& format : formats)
	{
		u8 rank = 0xff;
		if (format.format == desiredFormat)
		{
			--rank;
		}
		if (format.colorSpace == desiredColourSpace)
		{
			--rank;
		}
		availableFormats.emplace(rank, format);
	}
	return availableFormats.empty() ? vk::SurfaceFormatKHR() : availableFormats.begin()->second;
}

vk::PresentModeKHR Context::Info::bestPresentMode() const
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

vk::Extent2D Context::Info::extent(glm::ivec2 const& framebufferSize) const
{
	if (capabilities.currentExtent.width != maxVal<u32>())
	{
		return capabilities.currentExtent;
	}
	else
	{
		return {(u32)framebufferSize.x, (u32)framebufferSize.y};
	}
}

Context::Context(ContextData const& data) : m_window(data.config.window)
{
	ASSERT(data.config.getNewSurface && data.config.getFramebufferSize, "Required callbacks are null!");
	m_info.data = data;
	m_info.refresh();
	if (!m_info.isReady() || !createSwapchain())
	{
		throw std::runtime_error("Failed to create Context!");
	}
	LOG_I("[{}:{}] constructed", s_tName, m_window);
}

Context::~Context()
{
	destroySwapchain();
	vkDestroy<vk::Instance>(m_info.surface);
	LOG_I("[{}:{}] destroyed", s_tName, m_window);
}

bool Context::recreateSwapchain()
{
	LOG_D("[{}:{}] recreating swapchain...", s_tName, m_window);
	destroySwapchain();
	auto prevSurface = m_info.surface;
	m_info.refresh();
	[[maybe_unused]] bool bReady = m_info.isReady();
	ASSERT(bReady, "Context not ready!");
	if (bReady && createSwapchain())
	{
		if (prevSurface != m_info.surface)
		{
			vkDestroy<vk::Instance>(prevSurface);
		}
		LOG_D("[{}:{}] ... swapchain recreated", s_tName, m_window);
		return true;
	}
	LOG_E("[{}:{}] Failed to recreate swapchain!", s_tName, m_window);
	return false;
}

Context::Swapchain const& Context::active() const
{
	return m_swapchain;
}

vk::RenderPassBeginInfo Context::acquireNextImage(vk::Semaphore wait, vk::Fence setInUse)
{
	[[maybe_unused]] auto result = g_info.device.acquireNextImageKHR(m_swapchain.swapchain, maxVal<u64>(), wait, {});
	if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR)
	{
		LOG_D("[{}:{}] Failed to acquire next image [{}]", s_tName, m_window, g_vkResultStr[result.result]);
		m_info.refresh();
		recreateSwapchain();
		return acquireNextImage(wait, setInUse);
	}
	size_t const idx = (size_t)result.value;
	if (m_swapchain.frames.at(idx).drawing != vk::Fence())
	{
		g_info.device.waitForFences(m_swapchain.frames.at(idx).drawing, true, maxVal<u64>());
	}
	m_swapchain.frames.at(idx).drawing = setInUse;
	m_currentFrameIndex = (u32)idx;
	vk::RenderPassBeginInfo ret;
	ret.renderPass = m_swapchain.renderPass;
	ret.framebuffer = m_swapchain.frames.at(idx).framebuffer;
	ret.renderArea.extent = m_swapchain.extent;
	return ret;
}

vk::Result Context::present(vk::Semaphore wait)
{
	vk::PresentInfoKHR presentInfo;
	std::array swapchains = {m_swapchain.swapchain};
	auto const index = m_currentFrameIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &wait;
	presentInfo.swapchainCount = (u32)swapchains.size();
	presentInfo.pSwapchains = swapchains.data();
	presentInfo.pImageIndices = &index;
	auto result = g_info.queues.present.presentKHR(&presentInfo);
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
	{
		LOG_D("[{}:{}] Failed to present image [{}]", s_tName, m_window, g_vkResultStr[result]);
		m_info.refresh();
		recreateSwapchain();
	}
	return result;
}

bool Context::createSwapchain()
{
	// Swapchain
	auto size = m_info.data.config.getFramebufferSize();
	{
		vk::SwapchainCreateInfoKHR createInfo;
		createInfo.minImageCount = m_info.capabilities.minImageCount + 1;
		if (m_info.capabilities.maxImageCount != 0 && createInfo.minImageCount > m_info.capabilities.maxImageCount)
		{
			createInfo.minImageCount = m_info.capabilities.maxImageCount;
		}
		auto const format = m_info.bestColourFormat();
		m_colourFormat = format.format;
		createInfo.imageFormat = m_colourFormat;
		createInfo.imageColorSpace = format.colorSpace;
		m_swapchain.extent = m_info.extent(size);
		createInfo.imageExtent = m_swapchain.extent;
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
		createInfo.preTransform = m_info.capabilities.currentTransform;
		createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		createInfo.presentMode = m_info.bestPresentMode();
		createInfo.clipped = vk::Bool32(true);
		createInfo.surface = m_info.surface;
		m_swapchain.swapchain = g_info.device.createSwapchainKHR(createInfo);
	}
	// RenderPass
	{
		// TODO: Depth
		std::array<vk::AttachmentDescription, 1> descriptions;
		descriptions.at(0).format = m_colourFormat;
		descriptions.at(0).samples = vk::SampleCountFlagBits::e1;
		descriptions.at(0).loadOp = vk::AttachmentLoadOp::eClear;
		descriptions.at(0).stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		descriptions.at(0).stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		descriptions.at(0).initialLayout = vk::ImageLayout::eUndefined;
		descriptions.at(0).finalLayout = vk::ImageLayout::ePresentSrcKHR;
		vk::AttachmentReference colourRef;
		colourRef.attachment = 0;
		colourRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
		vk::SubpassDescription subpass;
		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colourRef;
		vk::RenderPassCreateInfo createInfo;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = descriptions.data();
		createInfo.subpassCount = (u32)descriptions.size();
		createInfo.pSubpasses = &subpass;
		vk::SubpassDependency dependency;
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;
		m_swapchain.renderPass = g_info.device.createRenderPass(createInfo);
	}
	// Frames
	{
		m_swapchain.swapchainImages = g_info.device.getSwapchainImagesKHR(m_swapchain.swapchain);
		m_swapchain.frames.reserve(m_swapchain.swapchainImages.size());
		for (auto const& image : m_swapchain.swapchainImages)
		{
			Frame frame;
			// Colour
			{
				frame.colour = createImageView(image, m_colourFormat, vk::ImageAspectFlagBits::eColor);
			}
			// TODO: Depth
			// Framebuffer
			{
				std::array attachments = {frame.colour};
				vk::FramebufferCreateInfo createInfo;
				createInfo.attachmentCount = (u32)attachments.size();
				createInfo.pAttachments = attachments.data();
				createInfo.renderPass = m_swapchain.renderPass;
				createInfo.width = m_swapchain.extent.width;
				createInfo.height = m_swapchain.extent.height;
				createInfo.layers = 1;
				frame.framebuffer = g_info.device.createFramebuffer(createInfo);
			}
			m_swapchain.frames.push_back(std::move(frame));
		}
		if (m_swapchain.swapchainImages.empty())
		{
			return false;
		}
	}
	LOG_D("[{}:{}] Swapchain created [{}x{}]", s_tName, m_window, size.x, size.y);
	return true;
}

void Context::destroySwapchain()
{
	g_info.device.waitIdle();
	for (auto const& frame : m_swapchain.frames)
	{
		vkDestroy(frame.framebuffer);
		vkDestroy(frame.colour);
		vkDestroy(frame.depth);
	}
	vkDestroy(m_swapchain.renderPass);
	vkDestroy(m_swapchain.depthImage);
	if (m_swapchain.swapchain != vk::SwapchainKHR())
	{
		vkDestroy(m_swapchain.swapchain);
		LOG_D("[{}:{}] Swapchain destroyed", s_tName, m_window);
	}
	m_swapchain = Swapchain();
}
} // namespace le::vuk
