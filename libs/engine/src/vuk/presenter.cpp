#include <map>
#include "core/log.hpp"
#include "core/utils.hpp"
#include "presenter.hpp"
#include "info.hpp"
#include "utils.hpp"

namespace le::vuk
{
std::string const Presenter::s_tName = utils::tName<Presenter>();

void Presenter::Info::refresh()
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

bool Presenter::Info::isReady() const
{
	return !formats.empty() && !presentModes.empty();
}

vk::SurfaceFormatKHR Presenter::Info::bestColourFormat() const
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

vk::PresentModeKHR Presenter::Info::bestPresentMode() const
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

vk::Extent2D Presenter::Info::extent(glm::ivec2 const& windowSize) const
{
	if (capabilities.currentExtent.width != maxVal<u32>())
	{
		return capabilities.currentExtent;
	}
	else
	{
		return {std::clamp((u32)windowSize.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
				std::clamp((u32)windowSize.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
	}
}

Presenter::Swapchain::Frame& Presenter::Swapchain::frame()
{
	return frames.at(imageIndex);
}

Presenter::Presenter(PresenterData const& data) : m_window(data.config.window)
{
	ASSERT(data.config.getNewSurface && data.config.getFramebufferSize && data.config.getWindowSize, "Required callbacks are null!");
	m_info.data = data;
	m_info.refresh();
	if (!m_info.isReady())
	{
		vkDestroy<vk::Instance>(m_info.surface);
		throw std::runtime_error("Failed to create Presenter!");
	}
	m_info.colourFormat = m_info.bestColourFormat();
	try
	{
		createRenderPass();
		createSwapchain();
	}
	catch (std::exception const& e)
	{
		cleanup();
		std::string text = "Failed to create Presenter! ";
		text += e.what();
		throw std::runtime_error(text.data());
	}
	LOG_I("[{}:{}] constructed", s_tName, m_window);
}

Presenter::~Presenter()
{
	cleanup();
	LOG_I("[{}:{}] destroyed", s_tName, m_window);
}

void Presenter::onFramebufferResize()
{
	auto const size = m_info.data.config.getFramebufferSize();
	if (m_flags.isSet(Flag::eRenderPaused))
	{
		if (size.x > 0 && size.y > 0)
		{
			if (recreateSwapchain())
			{
				LOG_I("[{}:{}] Non-zero framebuffer size detected; recreated swapchain [{}x{}]; resuming rendering", s_tName, m_window,
					  size.x, size.y);
				m_flags.reset(Flag::eRenderPaused);
			}
		}
	}
	else
	{
		if (size.x <= 0 || size.y <= 0)
		{
			LOG_I("[{}:{}] Invalid framebuffer size detected [{}x{}] (minimised surface?); pausing rendering", s_tName, m_window, size.x,
				  size.y);
			m_flags.set(Flag::eRenderPaused);
		}
		else if ((s32)m_swapchain.extent.width != size.x || (s32)m_swapchain.extent.height != size.y)
		{
			auto oldExtent = m_swapchain.extent;
			if (recreateSwapchain())
			{
				LOG_I("[{}:{}] Mismatched framebuffer size detected [{}x{}]; recreated swapchain [{}x{}]", s_tName, m_window, size.x,
					  size.y, oldExtent.width, oldExtent.height);
			}
		}
	}
}

TResult<Presenter::DrawFrame> Presenter::acquireNextImage(vk::Semaphore setDrawReady, vk::Fence drawing)
{
	if (m_flags.isSet(Flag::eRenderPaused))
	{
		return {false, {}};
	}
	auto const acquire = g_info.device.acquireNextImageKHR(m_swapchain.swapchain, maxVal<u64>(), setDrawReady, {});
	if (acquire.result != vk::Result::eSuccess && acquire.result != vk::Result::eSuboptimalKHR)
	{
		LOG_D("[{}:{}] Failed to acquire next image [{}]", s_tName, m_window, g_vkResultStr[acquire.result]);
		recreateSwapchain();
		return {false, {}};
	}
	m_swapchain.imageIndex = (u32)acquire.value;
	auto& frame = m_swapchain.frame();
	if (!frame.bNascent)
	{
		vuk::wait(frame.drawing);
	}
	frame.bNascent = false;
	frame.drawing = drawing;
	return {true, {m_renderPass, frame.framebuffer, m_swapchain.extent}};
}

bool Presenter::present(vk::Semaphore wait)
{
	if (m_flags.isSet(Flag::eRenderPaused))
	{
		return false;
	}
	// Present
	vk::PresentInfoKHR presentInfo;
	auto const index = m_swapchain.imageIndex;
	presentInfo.waitSemaphoreCount = 1U;
	presentInfo.pWaitSemaphores = &wait;
	presentInfo.swapchainCount = 1U;
	presentInfo.pSwapchains = &m_swapchain.swapchain;
	presentInfo.pImageIndices = &index;
	auto const result = g_info.queues.present.presentKHR(&presentInfo);
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
	{
		LOG_D("[{}:{}] Failed to present image [{}]", s_tName, m_window, g_vkResultStr[result]);
		recreateSwapchain();
		return false;
	}
	return true;
}

Presenter::OnEvent::Token Presenter::registerSwapchainRecreated(OnEvent::Callback callback)
{
	return m_onSwapchainRecreated.subscribe(callback);
}

Presenter::OnEvent::Token Presenter::registerDestroyed(OnEvent::Callback callback)
{
	return m_onDestroyed.subscribe(callback);
}

void Presenter::createRenderPass()
{
	std::array<vk::AttachmentDescription, 2> descriptions;
	vk::AttachmentReference colourAttachment, depthAttachment;
	{
		descriptions.at(0).format = m_info.colourFormat.format;
		descriptions.at(0).samples = vk::SampleCountFlagBits::e1;
		descriptions.at(0).loadOp = vk::AttachmentLoadOp::eClear;
		descriptions.at(0).stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		descriptions.at(0).stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		descriptions.at(0).initialLayout = vk::ImageLayout::eUndefined;
		descriptions.at(0).finalLayout = vk::ImageLayout::ePresentSrcKHR;
		colourAttachment.attachment = 0;
		colourAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
	}
	{
		descriptions.at(1).format = m_info.depthFormat;
		descriptions.at(1).samples = vk::SampleCountFlagBits::e1;
		descriptions.at(1).loadOp = vk::AttachmentLoadOp::eClear;
		descriptions.at(1).storeOp = vk::AttachmentStoreOp::eDontCare;
		descriptions.at(1).stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		descriptions.at(1).stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		descriptions.at(1).initialLayout = vk::ImageLayout::eUndefined;
		descriptions.at(1).finalLayout = vk::ImageLayout::ePresentSrcKHR;
		depthAttachment.attachment = 1;
		depthAttachment.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	}
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachment;
	subpass.pDepthStencilAttachment = &depthAttachment;
	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = (u32)descriptions.size();
	createInfo.pAttachments = descriptions.data();
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
	m_renderPass = g_info.device.createRenderPass(createInfo);
	return;
}

bool Presenter::createSwapchain()
{
	auto prevSurface = m_info.surface;
	m_info.refresh();
	[[maybe_unused]] bool bReady = m_info.isReady();
	ASSERT(bReady, "Presenter not ready!");
	// Swapchain
	m_info.refresh();
	auto const framebufferSize = m_info.data.config.getFramebufferSize();
	if (framebufferSize.x == 0 || framebufferSize.y == 0)
	{
		LOG_I("[{}:{}] Null framebuffer size detected (minimised surface?); pausing rendering", s_tName, m_window);
		m_flags.set(Flag::eRenderPaused);
		return false;
	}
	{
		vk::SwapchainCreateInfoKHR createInfo;
		createInfo.minImageCount = m_info.capabilities.minImageCount + 1;
		if (m_info.capabilities.maxImageCount != 0 && createInfo.minImageCount > m_info.capabilities.maxImageCount)
		{
			createInfo.minImageCount = m_info.capabilities.maxImageCount;
		}
		createInfo.imageFormat = m_info.colourFormat.format;
		createInfo.imageColorSpace = m_info.colourFormat.colorSpace;
		auto const windowSize = m_info.data.config.getWindowSize();
		m_swapchain.extent = m_info.extent(windowSize);
		createInfo.imageExtent = m_swapchain.extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		auto flags = vuk::Info::QFlags(vuk::Info::QFlag::eGraphics, vuk::Info::QFlag::eTransfer);
		auto const queueIndices = g_info.uniqueQueueIndices(flags);
		createInfo.pQueueFamilyIndices = queueIndices.data();
		createInfo.queueFamilyIndexCount = (u32)queueIndices.size();
		createInfo.imageSharingMode = g_info.sharingMode(flags);
		createInfo.preTransform = m_info.capabilities.currentTransform;
		createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		createInfo.presentMode = m_info.bestPresentMode();
		createInfo.clipped = vk::Bool32(true);
		createInfo.surface = m_info.surface;
		m_swapchain.swapchain = g_info.device.createSwapchainKHR(createInfo);
	}
	// Frames
	{
		auto images = g_info.device.getSwapchainImagesKHR(m_swapchain.swapchain);
		m_swapchain.frames.reserve(images.size());
		ImageData depthImageData;
		depthImageData.format = m_info.depthFormat;
		depthImageData.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
		depthImageData.size = vk::Extent3D(m_swapchain.extent, 1);
		depthImageData.tiling = vk::ImageTiling::eOptimal;
		depthImageData.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
		m_swapchain.depthImage = createImage(depthImageData);
		m_swapchain.depthImageView = createImageView(m_swapchain.depthImage.resource, m_info.depthFormat, vk::ImageAspectFlagBits::eDepth);
		for (auto const& image : images)
		{
			Swapchain::Frame frame;
			frame.image = image;
			frame.colour = createImageView(image, m_info.colourFormat.format);
			frame.depth = m_swapchain.depthImageView;
			{
				// Framebuffers
				std::array const attachments = {frame.colour, frame.depth};
				vk::FramebufferCreateInfo createInfo;
				createInfo.attachmentCount = (u32)attachments.size();
				createInfo.pAttachments = attachments.data();
				createInfo.renderPass = m_renderPass;
				createInfo.width = m_swapchain.extent.width;
				createInfo.height = m_swapchain.extent.height;
				createInfo.layers = 1;
				frame.framebuffer = g_info.device.createFramebuffer(createInfo);
			}
			m_swapchain.frames.push_back(std::move(frame));
		}
		if (m_swapchain.frames.empty())
		{
			throw std::runtime_error("Failed to create swapchain!");
		}
	}
	if (prevSurface != m_info.surface)
	{
		vkDestroy<vk::Instance>(prevSurface);
	}
	LOG_D("[{}:{}] Swapchain created [{}x{}]", s_tName, m_window, framebufferSize.x, framebufferSize.y);
	return true;
}

void Presenter::destroySwapchain()
{
	g_info.device.waitIdle();
	// m_renderer.reset();
	for (auto const& frame : m_swapchain.frames)
	{
		vkDestroy(frame.framebuffer, frame.colour);
	}
	vkDestroy(m_swapchain.depthImageView, m_swapchain.depthImage.resource, m_swapchain.swapchain);
	vkFree(m_swapchain.depthImage.memory);
	LOGIF_D(m_swapchain.swapchain != vk::SwapchainKHR(), "[{}:{}] Swapchain destroyed", s_tName, m_window);
	m_swapchain = Swapchain();
	m_onDestroyed();
}

void Presenter::cleanup()
{
	destroySwapchain();
	// m_renderer.destroy();
	vkDestroy(m_renderPass);
	m_renderPass = vk::RenderPass();
	vkDestroy<vk::Instance>(m_info.surface);
	m_info.surface = vk::SurfaceKHR();
	m_onDestroyed();
}

bool Presenter::recreateSwapchain()
{
	LOG_D("[{}:{}] Recreating Swapchain...", s_tName, m_window);
	destroySwapchain();
	if (createSwapchain())
	{
		LOG_D("[{}:{}] ... Swapchain recreated", s_tName, m_window);
		m_onSwapchainRecreated();
		return true;
	}
	LOGIF_E(!m_flags.isSet(Flag::eRenderPaused), "[{}:{}] Failed to recreate swapchain!", s_tName, m_window);
	return false;
}
} // namespace le::vuk
