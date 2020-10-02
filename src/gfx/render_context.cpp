#include <map>
#include <core/log.hpp>
#include <core/maths.hpp>
#include <core/utils.hpp>
#include <gfx/deferred.hpp>
#include <gfx/device.hpp>
#include <gfx/render_context.hpp>
#include <gfx/vram.hpp>

namespace le::gfx {
std::string const RenderContext::s_tName = utils::tName<RenderContext>();

void RenderContext::Metadata::refresh() {
	if (surface == vk::SurfaceKHR() || !g_device.isValid(surface)) {
		surface = info.config.getNewSurface(g_instance.instance);
	}
	[[maybe_unused]] bool bValid = g_device.isValid(surface);
	ASSERT(bValid, "Invalid surface!");
	capabilities = g_instance.physicalDevice.getSurfaceCapabilitiesKHR(surface);
	colourFormats = g_instance.physicalDevice.getSurfaceFormatsKHR(surface);
	presentModes = g_instance.physicalDevice.getSurfacePresentModesKHR(surface);
}

bool RenderContext::Metadata::ready() const {
	return !colourFormats.empty() && !presentModes.empty();
}

vk::SurfaceFormatKHR RenderContext::Metadata::bestColourFormat() const {
	static std::array const s_defaultFormats = {vk::Format::eB8G8R8A8Srgb};
	static std::array const s_defaultColourSpaces = {vk::ColorSpaceKHR::eSrgbNonlinear};
	auto const& desiredFormats = info.options.formats.empty() ? s_defaultFormats : info.options.formats;
	auto const& desiredColourSpaces = info.options.colourSpaces.empty() ? s_defaultColourSpaces : info.options.colourSpaces;
	std::map<u32, vk::SurfaceFormatKHR> ranked;
	for (auto const& available : colourFormats) {
		u16 formatRank = 0;
		for (auto desired : desiredFormats) {
			if (desired == available.format) {
				break;
			}
			++formatRank;
		}
		u16 colourSpaceRank = 0;
		for (auto desired : desiredColourSpaces) {
			if (desired == available.colorSpace) {
				break;
			}
			++formatRank;
		}
		ranked.emplace(formatRank + colourSpaceRank, available);
	}
	return ranked.empty() ? vk::SurfaceFormatKHR() : ranked.begin()->second;
}

vk::Format RenderContext::Metadata::bestDepthFormat() const {
	static std::array const desired = {vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint};
	auto format = g_instance.supportedFormat(desired, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	return format ? *format : vk::Format::eD16Unorm;
}

vk::PresentModeKHR RenderContext::Metadata::bestPresentMode() const {
	static std::array const s_defaultPresentModes = {vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eFifo};
	auto const& desiredPresentModes = info.options.presentModes.empty() ? s_defaultPresentModes : info.options.presentModes;
	std::map<u32, vk::PresentModeKHR> ranked;
	for (auto const& available : presentModes) {
		u32 rank = 0;
		for (auto desired : desiredPresentModes) {
			if (available == desired) {
				break;
			}
			++rank;
		}
		ranked.emplace(rank, available);
	}
	return ranked.empty() ? vk::PresentModeKHR::eFifo : ranked.begin()->second;
}

vk::Extent2D RenderContext::Metadata::extent(glm::ivec2 const& windowSize) const {
	if (capabilities.currentExtent.width != maths::max<u32>()) {
		return capabilities.currentExtent;
	} else {
		return {std::clamp((u32)windowSize.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
				std::clamp((u32)windowSize.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
	}
}

RenderFrame& RenderContext::Swapchain::frame() {
	return frames.at(imageIndex);
}

RenderContext::RenderContext(ContextInfo const& info) : m_window(info.config.window) {
	m_name = fmt::format("{}:{}", s_tName, m_window);
	ASSERT(info.config.getNewSurface && info.config.getFramebufferSize && info.config.getWindowSize, "Required callbacks are null!");
	m_metadata.info = info;
	m_metadata.refresh();
	if (!m_metadata.ready()) {
		g_instance.destroy(m_metadata.surface);
		throw std::runtime_error("Failed to create RenderContext!");
	}
	m_metadata.colourFormat = m_metadata.bestColourFormat();
	m_metadata.depthFormat = m_metadata.bestDepthFormat();
	m_metadata.presentMode = m_metadata.bestPresentMode();
	try {
		createSwapchain();
	} catch (std::exception const& e) {
		cleanup();
		std::string text = "Failed to create RenderContext! ";
		text += e.what();
		throw std::runtime_error(text.data());
	}
	LOG_I("[{}] constructed", m_name);
}

RenderContext::~RenderContext() {
	cleanup();
	LOG_I("[{}:{}] destroyed", s_tName, m_window);
}

void RenderContext::onFramebufferResize() {
	m_flags.set(Flag::eOutOfDate);
	auto const size = m_metadata.info.config.getFramebufferSize();
	m_flags[Flag::eRenderPaused] = size.x == 0 || size.y == 0;
}

TResult<RenderTarget> RenderContext::acquireNextImage(vk::Semaphore setDrawReady, vk::Fence setOnDrawn) {
	if (m_flags.test(Flag::eRenderPaused)) {
		return {};
	}
	if (m_flags.test(Flag::eOutOfDate)) {
		recreateSwapchain();
		return {};
	}
	auto const acquire = g_device.device.acquireNextImageKHR(m_swapchain.swapchain, maths::max<u64>(), setDrawReady, {});
	if (acquire.result != vk::Result::eSuccess && acquire.result != vk::Result::eSuboptimalKHR) {
		LOG_D("[{}] Failed to acquire next image [{}]", m_name, g_vkResultStr[acquire.result]);
		recreateSwapchain();
		return {};
	}
	m_swapchain.imageIndex = (u32)acquire.value;
	auto& frame = m_swapchain.frame();
	g_device.waitFor(frame.drawn);
	frame.drawn = setOnDrawn;
	return frame.swapchain;
}

bool RenderContext::present(vk::Semaphore wait) {
	if (m_flags.test(Flag::eRenderPaused)) {
		return false;
	}
	if (m_flags.test(Flag::eOutOfDate)) {
		recreateSwapchain();
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
	auto const result = g_device.queues.present.queue.presentKHR(&presentInfo);
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
		LOG_D("[{}] Failed to present image [{}]", m_name, g_vkResultStr[result]);
		recreateSwapchain();
		return false;
	}
	return true;
}

vk::Format RenderContext::colourFormat() const {
	return m_metadata.colourFormat.format;
}

vk::Format RenderContext::depthFormat() const {
	return m_metadata.depthFormat;
}

bool RenderContext::createSwapchain() {
	auto prevSurface = m_metadata.surface;
	m_metadata.refresh();
	[[maybe_unused]] bool bReady = m_metadata.ready();
	ASSERT(bReady, "RenderContext not ready!");
	// Swapchain
	m_metadata.presentMode = m_metadata.bestPresentMode();
	auto const framebufferSize = m_metadata.info.config.getFramebufferSize();
	if (framebufferSize.x == 0 || framebufferSize.y == 0) {
		LOG_I("[{}] Null framebuffer size detected (minimised surface?); pausing rendering", m_name);
		m_flags.set(Flag::eRenderPaused);
		return false;
	}
	{
		vk::SwapchainCreateInfoKHR createInfo;
		createInfo.minImageCount = m_metadata.capabilities.minImageCount + 1;
		if (m_metadata.capabilities.maxImageCount != 0 && createInfo.minImageCount > m_metadata.capabilities.maxImageCount) {
			createInfo.minImageCount = m_metadata.capabilities.maxImageCount;
		}
		createInfo.imageFormat = m_metadata.colourFormat.format;
		createInfo.imageColorSpace = m_metadata.colourFormat.colorSpace;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		auto const indices = g_device.queueIndices(QFlag::eGraphics);
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.pQueueFamilyIndices = indices.data();
		createInfo.queueFamilyIndexCount = (u32)indices.size();
		createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		createInfo.presentMode = m_metadata.presentMode;
		createInfo.clipped = vk::Bool32(true);
		m_metadata.refresh();
		createInfo.surface = m_metadata.surface;
		createInfo.preTransform = m_metadata.capabilities.currentTransform;
		createInfo.oldSwapchain = m_retiring;
		auto const windowSize = m_metadata.info.config.getWindowSize();
		m_swapchain.extent = createInfo.imageExtent = m_metadata.extent(windowSize);
		m_swapchain.swapchain = g_device.device.createSwapchainKHR(createInfo);
		m_retiring = vk::SwapchainKHR();
	}
	// Frames
	{
		auto images = g_device.device.getSwapchainImagesKHR(m_swapchain.swapchain);
		m_swapchain.frames.reserve(images.size());
		ImageInfo depthImageInfo;
		depthImageInfo.createInfo.format = m_metadata.depthFormat;
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
		viewInfo.format = m_metadata.depthFormat;
		viewInfo.aspectFlags = vk::ImageAspectFlagBits::eDepth;
		m_swapchain.depthImageView = g_device.createImageView(viewInfo);
		viewInfo.format = m_metadata.colourFormat.format;
		viewInfo.aspectFlags = vk::ImageAspectFlagBits::eColor;
		for (auto const& image : images) {
			RenderFrame frame;
			frame.swapchain.colour.image = image;
			frame.swapchain.depth.image = m_swapchain.depthImage.image;
			viewInfo.image = image;
			frame.swapchain.colour.view = g_device.createImageView(viewInfo);
			frame.swapchain.depth.view = m_swapchain.depthImageView;
			frame.swapchain.extent = m_swapchain.extent;
			m_swapchain.frames.push_back(std::move(frame));
		}
		if (m_swapchain.frames.empty()) {
			throw std::runtime_error("Failed to create swapchain!");
		}
	}
	if (prevSurface != m_metadata.surface) {
		g_instance.destroy(prevSurface);
	}
	LOG_D("[{}] Swapchain created [{}x{}]", m_name, framebufferSize.x, framebufferSize.y);
	m_flags.reset(Flag::eOutOfDate | Flag::eRenderPaused);
	return true;
}

void RenderContext::destroySwapchain() {
	m_retiring = m_swapchain.swapchain;
	deferred::release([swapchain = m_swapchain, name = m_name]() {
		for (auto const& frame : swapchain.frames) {
			g_device.destroy(frame.swapchain.colour.view);
		}
		g_device.destroy(swapchain.depthImageView, swapchain.swapchain);
		vram::release(swapchain.depthImage);
		LOGIF_D(swapchain.swapchain != vk::SwapchainKHR(), "[{}] Swapchain destroyed", name);
	});
	m_swapchain = {};
}

void RenderContext::cleanup() {
	destroySwapchain();
	g_device.waitIdle();
	g_instance.destroy(m_metadata.surface);
	m_metadata.surface = vk::SurfaceKHR();
}

bool RenderContext::recreateSwapchain() {
	LOG_D("[{}] Recreating Swapchain...", m_name);
	destroySwapchain();
	if (createSwapchain()) {
		LOG_D("[{}] ... Swapchain recreated", m_name);
		return true;
	}
	LOGIF_E(!m_flags.test(Flag::eRenderPaused), "[{}] Failed to recreate swapchain!", m_name);
	return false;
}
} // namespace le::gfx
