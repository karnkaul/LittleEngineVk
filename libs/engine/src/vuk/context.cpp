#include <map>
#include "core/log.hpp"
#include "core/utils.hpp"
#include "context.hpp"
#include "info.hpp"
#include "utils.hpp"

namespace le::vuk
{
std::string const Context::s_tName = utils::tName<Context>();

Context::Shared const Context::s_shared = {UBOData{sizeof(ubo::View)}};

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

Context::SwapchainFrame& Context::Swapchain::swapchainFrame()
{
	return frames.at(currentImageIndex);
}

Context::RenderSync::FrameSync& Context::RenderSync::frameSync()
{
	ASSERT(index < frames.size(), "Invalid index!");
	return frames.at(index);
}

void Context::RenderSync::next()
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
		vkDestroy<vk::Instance>(m_info.surface);
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

void Context::onFramebufferResize()
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

bool Context::renderFrame(Write write, Draw draw, BeginPass const& pass)
{
	if (m_flags.isSet(Flag::eRenderPaused) || !draw)
	{
		return false;
	}
	auto& frameSync = m_sync.frameSync();
	vuk::wait(frameSync.drawing);
	// Acquire
	auto const acquire = g_info.device.acquireNextImageKHR(m_swapchain.swapchain, maxVal<u64>(), frameSync.render, {});
	if (acquire.result != vk::Result::eSuccess && acquire.result != vk::Result::eSuboptimalKHR)
	{
		LOG_D("[{}:{}] Failed to acquire next image [{}]", s_tName, m_window, g_vkResultStr[acquire.result]);
		recreateSwapchain();
		return false;
	}
	m_swapchain.currentImageIndex = (u32)acquire.value;
	auto& swapchainFrame = m_swapchain.swapchainFrame();
	vuk::wait(swapchainFrame.drawing);
	swapchainFrame.commandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
	std::array<vk::ClearValue, 2> clearValues = {pass.colour, pass.depth};
	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = swapchainFrame.framebuffer;
	renderPassInfo.renderArea.extent = m_swapchain.extent;
	renderPassInfo.clearValueCount = (u32)clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();
	FrameDriver::UBOs ubos{swapchainFrame.ubos.view};
	if (write)
	{
		write(ubos);
	}
	// Begin
	auto const& commandBuffer = swapchainFrame.commandBuffer;
	vk::CommandBufferBeginInfo beginInfo;
	commandBuffer.begin(beginInfo);
	commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	FrameDriver driver{ubos, commandBuffer};
	draw(driver);
	commandBuffer.endRenderPass();
	commandBuffer.end();
	// Submit
	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frameSync.render;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &swapchainFrame.commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frameSync.present;
	g_info.device.resetFences(frameSync.drawing);
	g_info.queues.graphics.submit(submitInfo, frameSync.drawing);
	swapchainFrame.drawing = frameSync.drawing;
	// Present
	vk::PresentInfoKHR presentInfo;
	auto const index = m_swapchain.currentImageIndex;
	presentInfo.waitSemaphoreCount = 1U;
	presentInfo.pWaitSemaphores = &frameSync.present;
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
	m_sync.next();
	return true;
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

bool Context::createSwapchain()
{
	// Swapchain
	auto size = m_info.data.config.getFramebufferSize();
	if (size.x == 0 || size.y == 0)
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
		m_swapchain.extent = m_info.extent(size);
		createInfo.imageExtent = m_swapchain.extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		auto const indices = g_info.uniqueQueueIndices(true, false);
		createInfo.pQueueFamilyIndices = indices.data();
		createInfo.queueFamilyIndexCount = (u32)indices.size();
		createInfo.imageSharingMode = g_info.sharingMode(true, false);
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
			createImageView(m_swapchain.depthImage.resource, vk::ImageViewType::e2D, m_info.depthFormat, vk::ImageAspectFlagBits::eDepth);
		for (auto const& image : m_swapchain.swapchainImages)
		{
			auto imageView = createImageView(image, vk::ImageViewType::e2D, m_info.colourFormat.format, vk::ImageAspectFlagBits::eColor);
			m_swapchain.swapchainImageViews.push_back(imageView);
			SwapchainFrame frame;
			frame.colour = imageView;
			frame.depth = m_swapchain.depthImageView;
			{
				// Framebuffers
				std::array attachments = {frame.colour, frame.depth};
				vk::FramebufferCreateInfo createInfo;
				createInfo.attachmentCount = (u32)attachments.size();
				createInfo.pAttachments = attachments.data();
				createInfo.renderPass = m_renderPass;
				createInfo.width = m_swapchain.extent.width;
				createInfo.height = m_swapchain.extent.height;
				createInfo.layers = 1;
				frame.framebuffer = g_info.device.createFramebuffer(createInfo);
			}
			{
				// Commands
				vk::CommandPoolCreateInfo commandPoolCreateInfo;
				commandPoolCreateInfo.queueFamilyIndex = g_info.queueFamilyIndices.graphics;
				commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
				frame.commandPool = g_info.device.createCommandPool(commandPoolCreateInfo);
				vk::CommandBufferAllocateInfo allocInfo;
				allocInfo.commandPool = frame.commandPool;
				allocInfo.level = vk::CommandBufferLevel::ePrimary;
				allocInfo.commandBufferCount = 1;
				frame.commandBuffer = g_info.device.allocateCommandBuffers(allocInfo).front();
			}
			{
				// UBOs
				BufferData data;
				data.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
				data.usage = vk::BufferUsageFlagBits::eUniformBuffer;
				// Matrices
				data.size = s_shared.uboView.size;
				frame.ubos.view.buffer = createBuffer(data);
				frame.ubos.view.offset = 0;
			}
			m_swapchain.frames.push_back(std::move(frame));
		}
		if (m_swapchain.swapchainImages.empty() || m_swapchain.frames.empty())
		{
			throw std::runtime_error("Failed to create swapchain!");
		}
		m_frameCount = (u32)m_swapchain.frames.size();
	}
	// Descriptors
	{
		// Pool
		vk::DescriptorPoolSize descPoolSize;
		descPoolSize.type = vk::DescriptorType::eUniformBuffer;
		descPoolSize.descriptorCount = m_frameCount;
		vk::DescriptorPoolCreateInfo createInfo;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &descPoolSize;
		createInfo.maxSets = m_frameCount;
		m_swapchain.descriptorPool = g_info.device.createDescriptorPool(createInfo);
		// Sets
		vk::DescriptorSetAllocateInfo allocInfo;
		allocInfo.descriptorPool = m_swapchain.descriptorPool;
		allocInfo.descriptorSetCount = m_frameCount;
		std::vector layouts((size_t)m_frameCount, g_info.matricesLayout);
		allocInfo.pSetLayouts = layouts.data();
		auto sets = g_info.device.allocateDescriptorSets(allocInfo);
		for (size_t idx = 0; idx < sets.size(); ++idx)
		{
			auto& swapchainFrame = m_swapchain.frames.at(idx);
			swapchainFrame.ubos.view.descriptorSet = sets.at(idx);
			vk::DescriptorBufferInfo bufferInfo;
			bufferInfo.buffer = swapchainFrame.ubos.view.buffer.resource;
			bufferInfo.offset = 0;
			bufferInfo.range = s_shared.uboView.size;
			vk::WriteDescriptorSet descWrite;
			descWrite.dstSet = swapchainFrame.ubos.view.descriptorSet;
			descWrite.dstBinding = 0;
			descWrite.dstArrayElement = 0;
			descWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
			descWrite.descriptorCount = 1;
			descWrite.pBufferInfo = &bufferInfo;
			g_info.device.updateDescriptorSets(descWrite, {});
		}
	}
	m_sync = RenderSync();
	for (u32 i = 0; i < m_frameCount; ++i)
	{
		RenderSync::FrameSync frame;
		frame.render = g_info.device.createSemaphore({});
		frame.present = g_info.device.createSemaphore({});
		vk::FenceCreateInfo createInfo;
		createInfo.flags = vk::FenceCreateFlagBits::eSignaled;
		frame.drawing = g_info.device.createFence(createInfo);
		m_sync.frames.push_back(std::move(frame));
	}
	LOG_D("[{}:{}] Swapchain created [{}x{}]", s_tName, m_window, size.x, size.y);
	return true;
}

void Context::destroySwapchain()
{
	g_info.device.waitIdle();
	for (auto& frame : m_sync.frames)
	{
		vkDestroy(frame.drawing, frame.render, frame.present);
	}
	for (auto const& frame : m_swapchain.frames)
	{
		vkDestroy(frame.framebuffer, frame.commandPool, frame.ubos.view.buffer);
	}
	for (auto const& imageView : m_swapchain.swapchainImageViews)
	{
		vkDestroy(imageView);
	}
	vkDestroy(m_swapchain.descriptorPool);
	vkDestroy(m_swapchain.depthImageView, m_swapchain.depthImage.resource, m_swapchain.swapchain);
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
	LOG_D("[{}:{}] Recreating Swapchain...", s_tName, m_window);
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
		LOG_D("[{}:{}] ... Swapchain recreated", s_tName, m_window);
		return true;
	}
	LOGIF_E(!m_flags.isSet(Flag::eRenderPaused), "[{}:{}] Failed to recreate swapchain!", s_tName, m_window);
	return false;
}
} // namespace le::vuk
