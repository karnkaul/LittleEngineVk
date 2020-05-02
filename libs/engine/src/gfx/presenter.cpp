#include <map>
#include "core/log.hpp"
#include "core/utils.hpp"
#include "presenter.hpp"
#include "info.hpp"
#include "utils.hpp"
#include "vram.hpp"

namespace le::gfx
{
std::string const Presenter::s_tName = utils::tName<Presenter>();

void Presenter::Info::refresh()
{
	if (surface == vk::SurfaceKHR() || !g_info.isValid(surface))
	{
		surface = info.config.getNewSurface(g_info.instance);
	}
	[[maybe_unused]] bool bValid = g_info.isValid(surface);
	ASSERT(bValid, "Invalid surface!");
	capabilities = g_info.physicalDevice.getSurfaceCapabilitiesKHR(surface);
	colourFormats = g_info.physicalDevice.getSurfaceFormatsKHR(surface);
	presentModes = g_info.physicalDevice.getSurfacePresentModesKHR(surface);
}

bool Presenter::Info::isReady() const
{
	return !colourFormats.empty() && !presentModes.empty();
}

vk::SurfaceFormatKHR Presenter::Info::bestColourFormat() const
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

vk::Format Presenter::Info::bestDepthFormat() const
{
	static PriorityList<vk::Format> const desired = {vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint};
	auto [format, bResult] = supportedFormat(desired, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
	return bResult ? format : vk::Format::eD16Unorm;
}

vk::PresentModeKHR Presenter::Info::bestPresentMode() const
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

Presenter::Presenter(PresenterInfo const& info) : m_window(info.config.window)
{
	m_name = fmt::format("{}:{}", s_tName, m_window);
	ASSERT(info.config.getNewSurface && info.config.getFramebufferSize && info.config.getWindowSize, "Required callbacks are null!");
	m_info.info = info;
	m_info.refresh();
	if (!m_info.isReady())
	{
		vkDestroy<vk::Instance>(m_info.surface);
		throw std::runtime_error("Failed to create Presenter!");
	}
	m_info.colourFormat = m_info.bestColourFormat();
	m_info.depthFormat = m_info.bestDepthFormat();
	m_info.presentMode = m_info.bestPresentMode();
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
	LOG_I("[{}] constructed", m_name, m_window);
}

Presenter::~Presenter()
{
	cleanup();
	LOG_I("[{}:{}] destroyed", s_tName, m_window);
}

void Presenter::onFramebufferResize()
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

TResult<Presenter::DrawFrame> Presenter::acquireNextImage(vk::Semaphore setDrawReady, vk::Fence drawing)
{
	if (m_flags.isSet(Flag::eRenderPaused))
	{
		return {};
	}
	auto const acquire = g_info.device.acquireNextImageKHR(m_swapchain.swapchain, maxVal<u64>(), setDrawReady, {});
	if (acquire.result != vk::Result::eSuccess && acquire.result != vk::Result::eSuboptimalKHR)
	{
		LOG_D("[{}] Failed to acquire next image [{}]", m_name, m_window, g_vkResultStr[acquire.result]);
		recreateSwapchain();
		return {};
	}
	m_swapchain.imageIndex = (u32)acquire.value;
	auto& frame = m_swapchain.frame();
	if (!frame.bNascent)
	{
		gfx::waitFor(frame.drawing);
	}
	frame.bNascent = false;
	frame.drawing = drawing;
	m_state = State::eRunning;
	return DrawFrame{m_renderPass, m_swapchain.extent, {frame.colour, frame.depth}};
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
		LOG_D("[{}] Failed to present image [{}]", m_name, m_window, g_vkResultStr[result]);
		recreateSwapchain();
		return false;
	}
	m_state = State::eRunning;
	return true;
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
		auto const queues = g_info.uniqueQueues(gfx::QFlag::eGraphics | gfx::QFlag::eTransfer);
		createInfo.imageSharingMode = queues.mode;
		createInfo.pQueueFamilyIndices = queues.indices.data();
		createInfo.queueFamilyIndexCount = (u32)queues.indices.size();
		createInfo.preTransform = m_info.capabilities.currentTransform;
		createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		createInfo.presentMode = m_info.presentMode;
		createInfo.clipped = vk::Bool32(true);
		createInfo.surface = m_info.surface;
		m_swapchain.swapchain = g_info.device.createSwapchainKHR(createInfo);
	}
	// Frames
	{
		auto images = g_info.device.getSwapchainImagesKHR(m_swapchain.swapchain);
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
		m_swapchain.depthImageView = createImageView(m_swapchain.depthImage.image, m_info.depthFormat, vk::ImageAspectFlagBits::eDepth);
		for (auto const& image : images)
		{
			Swapchain::Frame frame;
			frame.image = image;
			frame.colour = createImageView(image, m_info.colourFormat.format);
			frame.depth = m_swapchain.depthImageView;
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
	LOG_D("[{}] Swapchain created [{}x{}]", m_name, m_window, framebufferSize.x, framebufferSize.y);
	return true;
}

void Presenter::destroySwapchain()
{
	g_info.device.waitIdle();
	for (auto const& frame : m_swapchain.frames)
	{
		vkDestroy(frame.colour);
	}
	vkDestroy(m_swapchain.depthImageView, m_swapchain.swapchain);
	vram::release(m_swapchain.depthImage);
	LOGIF_D(m_swapchain.swapchain != vk::SwapchainKHR(), "[{}] Swapchain destroyed", m_name, m_window);
	m_swapchain = Swapchain();
	m_state = State::eSwapchainDestroyed;
}

void Presenter::cleanup()
{
	destroySwapchain();
	vkDestroy(m_renderPass);
	m_renderPass = vk::RenderPass();
	vkDestroy<vk::Instance>(m_info.surface);
	m_info.surface = vk::SurfaceKHR();
	m_state = State::eDestroyed;
}

bool Presenter::recreateSwapchain()
{
	LOG_D("[{}] Recreating Swapchain...", m_name, m_window);
	destroySwapchain();
	if (createSwapchain())
	{
		LOG_D("[{}] ... Swapchain recreated", m_name, m_window);
		m_state = State::eSwapchainRecreated;
		return true;
	}
	LOGIF_E(!m_flags.isSet(Flag::eRenderPaused), "[{}] Failed to recreate swapchain!", m_name, m_window);
	return false;
}
} // namespace le::gfx
