#include <map>
#include "core/log.hpp"
#include "core/utils.hpp"
#include "render_context.hpp"
#include "device.hpp"
#include "vram.hpp"

namespace le::gfx
{
std::string const RenderContext::s_tName = utils::tName<RenderContext>();

void RenderContext::Info::refresh()
{
	if (surface == vk::SurfaceKHR() || !g_device.isValid(surface))
	{
		surface = info.config.getNewSurface(g_instance.instance);
	}
	[[maybe_unused]] bool bValid = g_device.isValid(surface);
	ASSERT(bValid, "Invalid surface!");
	capabilities = g_instance.physicalDevice.getSurfaceCapabilitiesKHR(surface);
	colourFormats = g_instance.physicalDevice.getSurfaceFormatsKHR(surface);
	presentModes = g_instance.physicalDevice.getSurfacePresentModesKHR(surface);
}

bool RenderContext::Info::isReady() const
{
	return !colourFormats.empty() && !presentModes.empty();
}

vk::SurfaceFormatKHR RenderContext::Info::bestColourFormat() const
{
	static std::vector<vk::Format> const s_defaultFormats = {vk::Format::eB8G8R8A8Srgb};
	static std::vector<vk::ColorSpaceKHR> const s_defaultColourSpaces = {vk::ColorSpaceKHR::eSrgbNonlinear};
	auto const& desiredFormats = info.options.formats.empty() ? s_defaultFormats : info.options.formats;
	auto const& desiredColourSpaces = info.options.colourSpaces.empty() ? s_defaultColourSpaces : info.options.colourSpaces;
	std::map<u32, vk::SurfaceFormatKHR> ranked;
	for (auto const& available : colourFormats)
	{
		u16 formatRank = 0;
		for (auto desired : desiredFormats)
		{
			if (desired == available.format)
			{
				break;
			}
			++formatRank;
		}
		u16 colourSpaceRank = 0;
		for (auto desired : desiredColourSpaces)
		{
			if (desired == available.colorSpace)
			{
				break;
			}
			++formatRank;
		}
		ranked.emplace(formatRank + colourSpaceRank, available);
	}
	return ranked.empty() ? vk::SurfaceFormatKHR() : ranked.begin()->second;
}

vk::Format RenderContext::Info::bestDepthFormat() const
{
	static PriorityList<vk::Format> const desired = {vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint};
	auto [format, bResult] =
		g_instance.supportedFormat(desired, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	return bResult ? format : vk::Format::eD16Unorm;
}

vk::PresentModeKHR RenderContext::Info::bestPresentMode() const
{
	static std::vector<vk::PresentModeKHR> const s_defaultPresentModes = {vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifo};
	auto const& desiredPresentModes = info.options.presentModes.empty() ? s_defaultPresentModes : info.options.presentModes;
	std::map<u32, vk::PresentModeKHR> ranked;
	for (auto const& available : presentModes)
	{
		u32 rank = 0;
		for (auto desired : desiredPresentModes)
		{
			if (available == desired)
			{
				break;
			}
			++rank;
		}
		ranked.emplace(rank, available);
	}
	return ranked.empty() ? vk::PresentModeKHR::eFifo : ranked.begin()->second;
}

vk::Extent2D RenderContext::Info::extent(glm::ivec2 const& windowSize) const
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

RenderFrame& RenderContext::Swapchain::frame()
{
	return frames.at(imageIndex);
}

RenderContext::RenderContext(ContextInfo const& info) : m_window(info.config.window)
{
	m_name = fmt::format("{}:{}", s_tName, m_window);
	ASSERT(info.config.getNewSurface && info.config.getFramebufferSize && info.config.getWindowSize, "Required callbacks are null!");
	m_info.info = info;
	m_info.refresh();
	if (!m_info.isReady())
	{
		g_instance.destroy(m_info.surface);
		throw std::runtime_error("Failed to create RenderContext!");
	}
	m_info.colourFormat = m_info.bestColourFormat();
	m_info.depthFormat = m_info.bestDepthFormat();
	m_info.presentMode = m_info.bestPresentMode();
	try
	{
		createSwapchain();
	}
	catch (std::exception const& e)
	{
		cleanup();
		std::string text = "Failed to create RenderContext! ";
		text += e.what();
		throw std::runtime_error(text.data());
	}
	LOG_I("[{}] constructed", m_name, m_window);
}

RenderContext::~RenderContext()
{
	cleanup();
	LOG_I("[{}:{}] destroyed", s_tName, m_window);
}

void RenderContext::onFramebufferResize()
{
	auto const size = m_info.info.config.getFramebufferSize();
	if (m_flags.isSet(Flag::eRenderPaused))
	{
		if (size.x > 0 && size.y > 0)
		{
			if (recreateSwapchain())
			{
				LOG_I("[{}] Non-zero framebuffer size detected; recreated swapchain [{}x{}]; resuming rendering", m_name, m_window, size.x,
					  size.y);
				m_flags.reset(Flag::eRenderPaused);
			}
		}
	}
	else
	{
		if (size.x <= 0 || size.y <= 0)
		{
			LOG_I("[{}] Invalid framebuffer size detected [{}x{}] (minimised surface?); pausing rendering", m_name, m_window, size.x,
				  size.y);
			m_flags.set(Flag::eRenderPaused);
		}
		else if ((s32)m_swapchain.extent.width != size.x || (s32)m_swapchain.extent.height != size.y)
		{
			auto oldExtent = m_swapchain.extent;
			if (recreateSwapchain())
			{
				LOG_I("[{}] Mismatched framebuffer size detected [{}x{}]; recreated swapchain [{}x{}]", m_name, m_window, size.x, size.y,
					  oldExtent.width, oldExtent.height);
			}
		}
	}
}

RenderContext::TOutcome<RenderTarget> RenderContext::acquireNextImage(vk::Semaphore setDrawReady, vk::Fence setOnDrawn)
{
	if (m_flags.isSet(Flag::eRenderPaused))
	{
		return Outcome::ePaused;
	}
	auto const acquire = g_device.device.acquireNextImageKHR(m_swapchain.swapchain, maxVal<u64>(), setDrawReady, {});
	if (acquire.result != vk::Result::eSuccess && acquire.result != vk::Result::eSuboptimalKHR)
	{
		LOG_D("[{}] Failed to acquire next image [{}]", m_name, m_window, g_vkResultStr[acquire.result]);
		recreateSwapchain();
		return Outcome::eSwapchainRecreated;
	}
	m_swapchain.imageIndex = (u32)acquire.value;
	auto& frame = m_swapchain.frame();
	g_device.waitFor(frame.drawn);
	frame.drawn = setOnDrawn;
	return RenderTarget{frame.swapchain};
}

RenderContext::Outcome RenderContext::present(vk::Semaphore wait)
{
	if (m_flags.isSet(Flag::eRenderPaused))
	{
		return Outcome::ePaused;
	}
	// Present
	vk::PresentInfoKHR presentInfo;
	auto const index = m_swapchain.imageIndex;
	presentInfo.waitSemaphoreCount = 1U;
	presentInfo.pWaitSemaphores = &wait;
	presentInfo.swapchainCount = 1U;
	presentInfo.pSwapchains = &m_swapchain.swapchain;
	presentInfo.pImageIndices = &index;
	auto const result = g_device.queues.present.queue.presentKHR(&presentInfo);
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
	{
		LOG_D("[{}] Failed to present image [{}]", m_name, m_window, g_vkResultStr[result]);
		recreateSwapchain();
		return Outcome::eSwapchainRecreated;
	}
	return Outcome::eSuccess;
}

vk::Format RenderContext::colourFormat() const
{
	return m_info.colourFormat.format;
}

vk::Format RenderContext::depthFormat() const
{
	return m_info.depthFormat;
}

bool RenderContext::createSwapchain()
{
	auto prevSurface = m_info.surface;
	m_info.refresh();
	[[maybe_unused]] bool bReady = m_info.isReady();
	ASSERT(bReady, "RenderContext not ready!");
	// Swapchain
	m_info.refresh();
	auto const framebufferSize = m_info.info.config.getFramebufferSize();
	if (framebufferSize.x == 0 || framebufferSize.y == 0)
	{
		LOG_I("[{}] Null framebuffer size detected (minimised surface?); pausing rendering", m_name, m_window);
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
		auto const windowSize = m_info.info.config.getWindowSize();
		m_swapchain.extent = m_info.extent(windowSize);
		createInfo.imageExtent = m_swapchain.extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		auto const queues = g_device.uniqueQueues(QFlag::eGraphics | QFlag::eTransfer);
		createInfo.imageSharingMode = queues.mode;
		createInfo.pQueueFamilyIndices = queues.indices.data();
		createInfo.queueFamilyIndexCount = (u32)queues.indices.size();
		createInfo.preTransform = m_info.capabilities.currentTransform;
		createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		createInfo.presentMode = m_info.presentMode;
		createInfo.clipped = vk::Bool32(true);
		createInfo.surface = m_info.surface;
		m_swapchain.swapchain = g_device.device.createSwapchainKHR(createInfo);
	}
	// Frames
	{
		auto images = g_device.device.getSwapchainImagesKHR(m_swapchain.swapchain);
		m_swapchain.frames.reserve(images.size());
		ImageInfo depthImageInfo;
		depthImageInfo.createInfo.format = m_info.depthFormat;
		depthImageInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		depthImageInfo.createInfo.extent = vk::Extent3D(m_swapchain.extent, 1);
		depthImageInfo.createInfo.tiling = vk::ImageTiling::eOptimal;
		depthImageInfo.createInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
		depthImageInfo.createInfo.samples = vk::SampleCountFlagBits::e1;
		depthImageInfo.createInfo.imageType = vk::ImageType::e2D;
		depthImageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		depthImageInfo.createInfo.mipLevels = 1;
		depthImageInfo.createInfo.arrayLayers = 1;
		depthImageInfo.queueFlags = QFlag::eGraphics;
		depthImageInfo.name = m_name + "_depth";
		m_swapchain.depthImage = vram::createImage(depthImageInfo);
		ImageViewInfo viewInfo;
		viewInfo.image = m_swapchain.depthImage.image;
		viewInfo.format = m_info.depthFormat;
		viewInfo.aspectFlags = vk::ImageAspectFlagBits::eDepth;
		m_swapchain.depthImageView = g_device.createImageView(viewInfo);
		viewInfo.format = m_info.colourFormat.format;
		viewInfo.aspectFlags = vk::ImageAspectFlagBits::eColor;
		for (auto const& image : images)
		{
			RenderFrame frame;
			frame.swapchain.colour.image = image;
			frame.swapchain.depth.image = m_swapchain.depthImage.image;
			viewInfo.image = image;
			frame.swapchain.colour.view = g_device.createImageView(viewInfo);
			frame.swapchain.depth.view = m_swapchain.depthImageView;
			frame.swapchain.extent = m_swapchain.extent;
			m_swapchain.frames.push_back(std::move(frame));
		}
		if (m_swapchain.frames.empty())
		{
			throw std::runtime_error("Failed to create swapchain!");
		}
	}
	if (prevSurface != m_info.surface)
	{
		g_instance.destroy(prevSurface);
	}
	LOG_D("[{}] Swapchain created [{}x{}]", m_name, m_window, framebufferSize.x, framebufferSize.y);
	return true;
}

void RenderContext::destroySwapchain()
{
	g_device.waitIdle();
	for (auto const& frame : m_swapchain.frames)
	{
		g_device.destroy(frame.swapchain.colour.view);
	}
	g_device.destroy(m_swapchain.depthImageView, m_swapchain.swapchain);
	vram::release(m_swapchain.depthImage);
	LOGIF_D(m_swapchain.swapchain != vk::SwapchainKHR(), "[{}] Swapchain destroyed", m_name, m_window);
	m_swapchain = {};
}

void RenderContext::cleanup()
{
	destroySwapchain();
	g_instance.destroy(m_info.surface);
	m_info.surface = vk::SurfaceKHR();
}

bool RenderContext::recreateSwapchain()
{
	LOG_D("[{}] Recreating Swapchain...", m_name, m_window);
	destroySwapchain();
	if (createSwapchain())
	{
		LOG_D("[{}] ... Swapchain recreated", m_name, m_window);
		return true;
	}
	LOGIF_E(!m_flags.isSet(Flag::eRenderPaused), "[{}] Failed to recreate swapchain!", m_name, m_window);
	return false;
}
} // namespace le::gfx
