#include <glm/gtc/matrix_transform.hpp>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "info.hpp"
#include "renderer.hpp"
#include "utils.hpp"
#include "draw/pipeline.hpp"
#include "draw/resource_descriptors.hpp"

namespace le::gfx
{
std::string const Renderer::s_tName = utils::tName<Renderer>();

Renderer::Renderer(Info const& info) : m_presenter(info.presenterInfo), m_window(info.windowID)
{
	m_name = fmt::format("{}:{}", s_tName, m_window);
	create(info.frameCount);
}

Renderer::~Renderer()
{
	destroy();
}

void Renderer::create(u8 frameCount)
{
	if (m_frames.empty() && frameCount > 0)
	{
		m_frameCount = frameCount;
		// Descriptors
		auto descriptorSetup = rd::allocateSets(frameCount);
		ASSERT(descriptorSetup.set.size() == (size_t)frameCount, "Invalid setup!");
		m_descriptorPool = descriptorSetup.descriptorPool;
		for (u8 idx = 0; idx < frameCount; ++idx)
		{
			Renderer::FrameSync frame;
			frame.set = descriptorSetup.set.at((size_t)idx);
			frame.renderReady = g_info.device.createSemaphore({});
			frame.presentReady = g_info.device.createSemaphore({});
			frame.drawing = g_info.device.createFence({});
			// Commands
			vk::CommandPoolCreateInfo commandPoolCreateInfo;
			commandPoolCreateInfo.queueFamilyIndex = g_info.queueFamilyIndices.graphics;
			commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
			frame.commandPool = g_info.device.createCommandPool(commandPoolCreateInfo);
			vk::CommandBufferAllocateInfo allocInfo;
			allocInfo.commandPool = frame.commandPool;
			allocInfo.level = vk::CommandBufferLevel::ePrimary;
			allocInfo.commandBufferCount = 1;
			frame.commandBuffer = g_info.device.allocateCommandBuffers(allocInfo).front();
			m_frames.push_back(std::move(frame));
		}
		LOG_D("[{}] created", m_name);
	}
	return;
}

void Renderer::destroy()
{
	m_pipelines.clear();
	if (!m_frames.empty())
	{
		for (auto& frame : m_frames)
		{
			frame.set.destroy();
			vkDestroy(frame.commandPool, frame.framebuffer, frame.drawing, frame.renderReady, frame.presentReady);
		}
		vkDestroy(m_descriptorPool);
		m_descriptorPool = vk::DescriptorPool();
		m_frames.clear();
		m_index = 0;
		LOG_D("[{}] destroyed", m_name);
	}
	return;
}

void Renderer::reset()
{
	if (m_frameCount > 0)
	{
		create(m_frameCount);
		g_info.device.waitIdle();
		for (auto& frame : m_frames)
		{
			frame.bNascent = true;
			g_info.device.resetFences(frame.drawing);
			g_info.device.resetCommandPool(frame.commandPool, vk::CommandPoolResetFlagBits::eReleaseResources);
		}
		LOG_D("[{}] reset", m_name);
	}
	return;
}

Pipeline* Renderer::createPipeline(Pipeline::Info info)
{
	if (info.renderPass == vk::RenderPass())
	{
		info.renderPass = m_presenter.m_renderPass;
	}
	m_pipelines.push_back({});
	auto& pipeline = m_pipelines.back();
	if (!pipeline.create(std::move(info)))
	{
		m_pipelines.pop_back();
		return nullptr;
	}
	return &pipeline;
}

void Renderer::update()
{
	switch (m_presenter.m_state)
	{
	case Presenter::State::eDestroyed:
	{
		destroy();
		return;
	}
	case Presenter::State::eSwapchainDestroyed:
	{
		destroy();
		return;
	}
	case Presenter::State::eSwapchainRecreated:
	{
		destroy();
		create(m_frameCount);
		break;
	}
	default:
	{
		break;
	}
	}
	for (auto& pipeline : m_pipelines)
	{
		pipeline.update();
	}
	return;
}

bool Renderer::render(Write write, Draw draw, ClearValues const& clear)
{
	auto& frame = frameSync();
	if (!frame.bNascent)
	{
		gfx::waitFor(frame.drawing);
	}
	if (write)
	{
		write(frame.set);
	}
	// Acquire
	auto [acquire, bResult] = m_presenter.acquireNextImage(frame.renderReady, frame.drawing);
	if (!bResult)
	{
		return false;
	}
	// Framebuffer
	vkDestroy(frame.framebuffer);
	vk::FramebufferCreateInfo createInfo;
	createInfo.attachmentCount = (u32)acquire.attachments.size();
	createInfo.pAttachments = acquire.attachments.data();
	createInfo.renderPass = acquire.renderPass;
	createInfo.width = acquire.swapchainExtent.width;
	createInfo.height = acquire.swapchainExtent.height;
	createInfo.layers = 1;
	frame.framebuffer = g_info.device.createFramebuffer(createInfo);
	// RenderPass
	std::array const clearColour = {clear.colour.r.toF32(), clear.colour.g.toF32(), clear.colour.b.toF32(), clear.colour.a.toF32()};
	vk::ClearColorValue const colour = clearColour;
	vk::ClearDepthStencilValue const depth = {clear.depthStencil.x, (u32)clear.depthStencil.y};
	std::array<vk::ClearValue, 2> const clearValues = {colour, depth};
	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.renderPass = acquire.renderPass;
	renderPassInfo.framebuffer = frame.framebuffer;
	renderPassInfo.renderArea.extent = acquire.swapchainExtent;
	renderPassInfo.clearValueCount = (u32)clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();
	// Begin
	auto const& commandBuffer = frame.commandBuffer;
	vk::CommandBufferBeginInfo beginInfo;
	commandBuffer.begin(beginInfo);
	commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	FrameDriver driver{frame.set, commandBuffer};
	auto pipelines = draw(driver);
	commandBuffer.endRenderPass();
	commandBuffer.end();
	// Submit
	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags const waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &frame.renderReady;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &frame.presentReady;
	g_info.device.resetFences(frame.drawing);
	for (auto pPipeline : pipelines)
	{
		pPipeline->attach(frame.drawing);
	}
	g_info.queues.graphics.submit(submitInfo, frame.drawing);
	if (m_presenter.present(frame.presentReady))
	{
		next();
		return true;
	}
	return false;
}

vk::Viewport Renderer::transformViewport(ScreenRect const& nRect, glm::vec2 const& depth) const
{
	vk::Viewport viewport;
	auto const& extent = m_presenter.m_swapchain.extent;
	glm::vec2 const size = {nRect.right - nRect.left, nRect.bottom - nRect.top};
	viewport.minDepth = depth.x;
	viewport.maxDepth = depth.y;
	viewport.width = size.x * extent.width;
	viewport.height = size.y * extent.height;
	viewport.x = nRect.left * extent.width;
	viewport.y = nRect.top * extent.height;
	return viewport;
}

vk::Rect2D Renderer::transformScissor(ScreenRect const& nRect) const
{
	vk::Rect2D scissor;
	glm::vec2 const size = {nRect.right - nRect.left, nRect.bottom - nRect.top};
	scissor.offset.x = (s32)(nRect.left * m_presenter.m_swapchain.extent.width);
	scissor.offset.y = (s32)(nRect.top * m_presenter.m_swapchain.extent.height);
	scissor.extent.width = (u32)(size.x * m_presenter.m_swapchain.extent.width);
	scissor.extent.height = (u32)(size.y * m_presenter.m_swapchain.extent.height);
	return scissor;
}

void Renderer::onFramebufferResize()
{
	m_presenter.onFramebufferResize();
	return;
}

Renderer::FrameSync& Renderer::frameSync()
{
	ASSERT(m_index < m_frames.size(), "Invalid index!");
	return m_frames.at(m_index);
}

void Renderer::next()
{
	m_frames.at(m_index).bNascent = false;
	m_index = (m_index + 1) % m_frames.size();
	return;
}
} // namespace le::gfx
