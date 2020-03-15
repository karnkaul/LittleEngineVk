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

vuk::Context::RenderSync::FrameSync& vuk::Context::RenderSync::frameSync()
{
	ASSERT(index < frames.size(), "Invalid index!");
	return frames.at(index);
}

void vuk::Context::RenderSync::next()
{
	index = (index + 1) % (u32)frames.size();
}

Context::Context(ContextData const& data) : m_window(data.config.window)
{
	ASSERT(data.config.getNewSurface && data.config.getFramebufferSize, "Required callbacks are null!");
	m_info.data = data;
	m_info.refresh();
	if (!m_info.isReady())
	{
		throw std::runtime_error("Failed to create Context!");
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
		std::string text = "Failed to create Context! ";
		text += e.what();
		throw std::runtime_error(text.data());
	}
	LOG_I("[{}:{}] constructed", s_tName, m_window);
}

Context::~Context()
{
	cleanup();
	LOG_I("[{}:{}] destroyed", s_tName, m_window);
}

vk::Viewport Context::transformViewport(ScreenRect const& nRect, glm::vec2 const& depth) const
{
	vk::Viewport viewport;
	auto const& extent = m_swapchain.extent;
	glm::vec2 const size = {nRect.right - nRect.left, nRect.bottom - nRect.top};
	viewport.minDepth = depth.x;
	viewport.maxDepth = depth.y;
	viewport.width = size.x * extent.width;
	viewport.height = size.y * extent.height;
	viewport.x = nRect.left * extent.width;
	viewport.y = nRect.top * extent.height;
	return viewport;
}

vk::Rect2D Context::transformScissor(ScreenRect const& nRect) const
{
	vk::Rect2D scissor;
	glm::vec2 const size = {nRect.right - nRect.left, nRect.bottom - nRect.top};
	scissor.offset.x = (s32)(nRect.left * m_swapchain.extent.width);
	scissor.offset.y = (s32)(nRect.top * m_swapchain.extent.height);
	scissor.extent.width = (u32)(size.x * m_swapchain.extent.width);
	scissor.extent.height = (u32)(size.y * m_swapchain.extent.height);
	return scissor;
}

vk::CommandBuffer Context::beginRenderPass(BeginPass const& pass)
{
	auto& frame = m_sync.frameSync();
	if (frame.sync.drawing != vk::Fence())
	{
		g_info.device.waitForFences(frame.sync.drawing, true, maxVal<u64>());
	}
	g_info.device.resetCommandPool(frame.pool, vk::CommandPoolResetFlagBits::eReleaseResources);
	frame.renderPassInfo = acquireNextImage(frame.sync.render, frame.sync.drawing);
	// Begin
	auto const& commandBuffer = frame.commandBuffer;
	vk::CommandBufferBeginInfo beginInfo;
	commandBuffer.begin(beginInfo);
	std::array<vk::ClearValue, 2> clearValues = {pass.colour, pass.depth};
	frame.renderPassInfo.clearValueCount = (u32)clearValues.size();
	frame.renderPassInfo.pClearValues = clearValues.data();
	commandBuffer.beginRenderPass(frame.renderPassInfo, vk::SubpassContents::eInline);
	return commandBuffer;
}

void Context::submitPresent()
{
	auto const& frame = m_sync.frameSync();
	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frame.sync.render;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frame.commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frame.sync.present;
	g_info.device.resetFences(frame.sync.drawing);
	g_info.queues.graphics.front().submit(submitInfo, frame.sync.drawing);
	// Present
	auto const result = present(frame.sync.present);
	if (result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR)
	{
		m_sync.next();
	}
	return;
}

void Context::createRenderPass()
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

void Context::createSwapchain()
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
		createInfo.imageFormat = m_info.colourFormat.format;
		createInfo.imageColorSpace = m_info.colourFormat.colorSpace;
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
	// Frames
	{
		m_swapchain.swapchainImages = g_info.device.getSwapchainImagesKHR(m_swapchain.swapchain);
		m_swapchain.frames.reserve(m_swapchain.swapchainImages.size());
		ImageData depthImageData;
		depthImageData.format = m_info.depthFormat;
		depthImageData.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
		depthImageData.size = vk::Extent3D(m_swapchain.extent, 1);
		depthImageData.tiling = vk::ImageTiling::eOptimal;
		depthImageData.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
		m_swapchain.depthImage = createImage(depthImageData);
		m_swapchain.depthImageView =
			createImageView(m_swapchain.depthImage.image, vk::ImageViewType::e2D, m_info.depthFormat, vk::ImageAspectFlagBits::eDepth);
		for (auto const& image : m_swapchain.swapchainImages)
		{
			auto imageView = createImageView(image, vk::ImageViewType::e2D, m_info.colourFormat.format, vk::ImageAspectFlagBits::eColor);
			m_swapchain.swapchainImageViews.push_back(imageView);
			Frame frame;
			frame.colour = imageView;
			frame.depth = m_swapchain.depthImageView;
			std::array attachments = {frame.colour, frame.depth};
			vk::FramebufferCreateInfo createInfo;
			createInfo.attachmentCount = (u32)attachments.size();
			createInfo.pAttachments = attachments.data();
			createInfo.renderPass = m_renderPass;
			createInfo.width = m_swapchain.extent.width;
			createInfo.height = m_swapchain.extent.height;
			createInfo.layers = 1;
			frame.framebuffer = g_info.device.createFramebuffer(createInfo);
			m_swapchain.frames.push_back(std::move(frame));
		}
		if (m_swapchain.swapchainImages.empty() || m_swapchain.frames.empty())
		{
			throw std::runtime_error("Failed to create swapchain!");
		}
	}
	m_frameCount = (u32)m_swapchain.frames.size();
	m_sync = RenderSync();
	for (u32 i = 0; i < m_frameCount; ++i)
	{
		RenderSync::FrameSync frame;
		frame.sync.render = vuk::g_info.device.createSemaphore({});
		frame.sync.present = vuk::g_info.device.createSemaphore({});
		vk::FenceCreateInfo createInfo;
		createInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		frame.sync.drawing = vuk::g_info.device.createFence(createInfo);
		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		commandPoolCreateInfo.queueFamilyIndex = vuk::g_info.queueFamilyIndices.graphics;
		frame.pool = vuk::g_info.device.createCommandPool(commandPoolCreateInfo);
		vk::CommandBufferAllocateInfo allocInfo;
		allocInfo.commandPool = frame.pool;
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandBufferCount = 1;
		frame.commandBuffer = vuk::g_info.device.allocateCommandBuffers(allocInfo).front();
		m_sync.frames.push_back(std::move(frame));
	}
	LOG_D("[{}:{}] Swapchain created [{}x{}]", s_tName, m_window, size.x, size.y);
	return;
}

void Context::destroySwapchain()
{
	g_info.device.waitIdle();
	for (auto& frame : m_sync.frames)
	{
		g_info.device.destroy(frame.sync.present);
		g_info.device.destroy(frame.sync.render);
		g_info.device.destroy(frame.sync.drawing);
		g_info.device.destroy(frame.pool);
	}
	for (auto const& frame : m_swapchain.frames)
	{
		vkDestroy(frame.framebuffer);
	}
	for (auto const& imageView : m_swapchain.swapchainImageViews)
	{
		vkDestroy(imageView);
	}
	vkDestroy(m_swapchain.depthImageView, m_swapchain.depthImage.image, m_swapchain.swapchain);
	vkFree(m_swapchain.depthImage.memory);
	LOGIF_D(m_swapchain.swapchain != vk::SwapchainKHR(), "[{}:{}] Swapchain destroyed", s_tName, m_window);
	m_swapchain = Swapchain();
	m_sync = RenderSync();
	m_frameCount = 0;
}

void Context::cleanup()
{
	destroySwapchain();
	vkDestroy(m_renderPass);
	m_renderPass = vk::RenderPass();
	vkDestroy<vk::Instance>(m_info.surface);
	m_info.surface = vk::SurfaceKHR();
	m_frameCount = 0;
}

bool Context::recreateSwapchain()
{
	LOG_D("[{}:{}] recreating swapchain...", s_tName, m_window);
	destroySwapchain();
	auto prevSurface = m_info.surface;
	m_info.refresh();
	[[maybe_unused]] bool bReady = m_info.isReady();
	ASSERT(bReady, "Context not ready!");
	if (bReady)
	{
		createSwapchain();
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

vk::RenderPassBeginInfo Context::acquireNextImage(vk::Semaphore wait, vk::Fence setInUse)
{
	[[maybe_unused]] auto result = g_info.device.acquireNextImageKHR(m_swapchain.swapchain, maxVal<u64>(), wait, {});
	if (result.result != vk::Result::eSuccess && result.result != vk::Result::eSuboptimalKHR)
	{
		LOG_D("[{}:{}] Failed to acquire next image [{}]", s_tName, m_window, g_vkResultStr[result.result]);
		recreateSwapchain();
		return acquireNextImage(wait, setInUse);
	}
	size_t const idx = (size_t)result.value;
	if (m_swapchain.frames.at(idx).drawing != vk::Fence())
	{
		g_info.device.waitForFences(m_swapchain.frames.at(idx).drawing, true, maxVal<u64>());
	}
	m_swapchain.frames.at(idx).drawing = setInUse;
	m_swapchain.currentImageIndex = (u32)idx;
	vk::RenderPassBeginInfo ret;
	ret.renderPass = m_renderPass;
	ret.framebuffer = m_swapchain.frames.at(idx).framebuffer;
	ret.renderArea.extent = m_swapchain.extent;
	return ret;
}

vk::Result Context::present(vk::Semaphore wait)
{
	vk::PresentInfoKHR presentInfo;
	std::array swapchains = {m_swapchain.swapchain};
	auto const index = m_swapchain.currentImageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &wait;
	presentInfo.swapchainCount = (u32)swapchains.size();
	presentInfo.pSwapchains = swapchains.data();
	presentInfo.pImageIndices = &index;
	auto result = g_info.queues.present.presentKHR(&presentInfo);
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
	{
		LOG_D("[{}:{}] Failed to present image [{}]", s_tName, m_window, g_vkResultStr[result]);
		recreateSwapchain();
	}
	return result;
}
} // namespace le::vuk
