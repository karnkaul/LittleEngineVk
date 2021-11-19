#include <core/log.hpp>
#include <core/maths.hpp>
#include <glm/gtx/transform.hpp>
#include <graphics/common.hpp>
#include <graphics/context/defer_queue.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/context.hpp>
#include <map>
#include <stdexcept>

namespace le::graphics {
namespace {
void validateBuffering([[maybe_unused]] Buffering images, Buffering buffering) {
	ENSURE(images > 1_B, "Insufficient swapchain images");
	ENSURE(buffering > 0_B, "Insufficient buffering");
	if ((s16)buffering.value - (s16)images.value > 1) { g_log.log(lvl::warn, 0, "[{}] Buffering significantly more than swapchain image count", g_name); }
	if (buffering < 2_B) { g_log.log(lvl::warn, 0, "[{}] Buffering less than double; expect hitches", g_name); }
}
} // namespace

VertexInputInfo RenderContext::vertexInput(VertexInputCreateInfo const& info) {
	VertexInputInfo ret;
	u32 bindDelta = 0, locationDelta = 0;
	for (auto& type : info.types) {
		vk::VertexInputBindingDescription binding;
		binding.binding = u32(info.bindStart + bindDelta);
		binding.stride = (u32)type.size;
		binding.inputRate = type.inputRate;
		ret.bindings.push_back(binding);
		for (auto const& member : type.members) {
			vk::VertexInputAttributeDescription attribute;
			attribute.binding = u32(info.bindStart + bindDelta);
			attribute.format = member.format;
			attribute.offset = (u32)member.offset;
			attribute.location = u32(info.locationStart + locationDelta++);
			ret.attributes.push_back(attribute);
		}
		++bindDelta;
	}
	return ret;
}

VertexInputInfo RenderContext::vertexInput(QuickVertexInput const& info) {
	VertexInputInfo ret;
	vk::VertexInputBindingDescription binding;
	binding.binding = info.binding;
	binding.stride = (u32)info.size;
	binding.inputRate = vk::VertexInputRate::eVertex;
	ret.bindings.push_back(binding);
	u32 location = 0;
	for (auto const& [format, offset] : info.attributes) {
		vk::VertexInputAttributeDescription attribute;
		attribute.binding = info.binding;
		attribute.format = format;
		attribute.offset = (u32)offset;
		attribute.location = location++;
		ret.attributes.push_back(attribute);
	}
	return ret;
}

RenderContext::RenderContext(not_null<VRAM*> vram, Swapchain::CreateInfo const& info, glm::vec2 fbSize)
	: m_swapchain(vram, info, fbSize), m_pool(vram->m_device, vk::CommandPoolCreateFlagBits::eTransient), m_device(vram->m_device) {
	m_storage.pipelineCache = makeDeferred<vk::PipelineCache>(m_device);
}

void RenderContext::initRenderer() {
	m_storage.status = Status::eWaiting;
	validateBuffering(m_swapchain.buffering(), m_storage.renderer->buffering());
	DeferQueue::defaultDefer = m_storage.renderer->buffering();
}

Pipeline RenderContext::makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo info) {
	if (info.renderPass == vk::RenderPass()) { info.renderPass = m_storage.renderer->renderPass3D(); }
	info.buffering = m_storage.renderer->buffering();
	info.cache = m_storage.pipelineCache;
	return Pipeline(m_swapchain.m_vram, shader, std::move(info), id);
}

bool RenderContext::ready(glm::ivec2 framebufferSize) {
	if (m_storage.reconstruct.trigger || m_swapchain.flags().any(Swapchain::Flags(Swapchain::Flag::eOutOfDate) | Swapchain::Flag::ePaused)) {
		auto const& vsync = m_storage.reconstruct.vsync;
		bool const ret = vsync ? m_swapchain.reconstruct(*vsync, framebufferSize) : m_swapchain.reconstruct(framebufferSize);
		if (ret) {
			m_storage.renderer->refresh();
			m_storage.status = Status::eWaiting;
			m_storage.reconstruct = {};
		}
		return false;
	}
	return true;
}

bool RenderContext::waitForFrame() {
	if (!check(Status::eReady)) {
		if (m_storage.status != Status::eWaiting && m_storage.status != Status::eBegun) {
			g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
			return false;
		}
		m_storage.renderer->waitForFrame();
		m_pool.update();
		set(Status::eReady);
	}
	return true;
}

std::optional<RenderTarget> RenderContext::beginFrame() {
	if (!check(Status::eReady)) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return std::nullopt;
	}
	if (m_swapchain.flags().any(Swapchain::Flags(Swapchain::Flag::ePaused) | Swapchain::Flag::eOutOfDate)) { return std::nullopt; }
	if (auto ret = m_storage.renderer->beginFrame()) {
		set(Status::eBegun);
		m_storage.drawing = *ret;
		return ret;
	}
	return std::nullopt;
}

std::optional<CommandBuffer> RenderContext::beginDraw(ScreenView const& view, RGBA clear, ClearDepth depth) {
	if (!check(Status::eBegun) || !m_storage.drawing) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return std::nullopt;
	}
	set(Status::eDrawing);
	return m_storage.renderer->beginDraw(*m_storage.drawing, view, clear, depth);
}

bool RenderContext::endDraw() {
	if (!m_storage.drawing || !check(Status::eDrawing)) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	m_storage.renderer->endDraw(*m_storage.drawing);
	return true;
}

bool RenderContext::endFrame() {
	if (!check(Status::eDrawing, Status::eBegun)) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	set(Status::eEnded);
	m_storage.drawing.reset();
	m_storage.renderer->endFrame();
	return true;
}

bool RenderContext::submitFrame() {
	if (!check(Status::eEnded)) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	set(Status::eWaiting);
	if (m_storage.renderer->submitFrame()) { return true; }
	return false;
}

void RenderContext::reconstruct(std::optional<graphics::Vsync> vsync) {
	m_storage.reconstruct.trigger = true;
	m_storage.reconstruct.vsync = vsync;
}

glm::mat4 RenderContext::preRotate() const noexcept {
	glm::mat4 ret(1.0f);
	f32 rad = 0.0f;
	auto const transform = m_swapchain.display().transform;
	if (transform == vk::SurfaceTransformFlagBitsKHR::eIdentity) {
		return ret;
	} else if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate90) {
		rad = glm::radians(90.0f);
	} else if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate180) {
		rad = glm::radians(180.0f);
	} else if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate180) {
		rad = glm::radians(270.0f);
	}
	return glm::rotate(ret, rad, front);
}

vk::Viewport RenderContext::viewport(Extent2D extent, ScreenView const& view, glm::vec2 depth) const noexcept {
	return m_storage.renderer->viewport(Swapchain::valid(extent) ? extent : this->extent(), view, depth);
}

vk::Rect2D RenderContext::scissor(Extent2D extent, ScreenView const& view) const noexcept {
	return m_storage.renderer->scissor(Swapchain::valid(extent) ? extent : this->extent(), view);
}

namespace foo {

VertexInputInfo RenderContext::vertexInput(VertexInputCreateInfo const& info) {
	VertexInputInfo ret;
	u32 bindDelta = 0, locationDelta = 0;
	for (auto& type : info.types) {
		vk::VertexInputBindingDescription binding;
		binding.binding = u32(info.bindStart + bindDelta);
		binding.stride = (u32)type.size;
		binding.inputRate = type.inputRate;
		ret.bindings.push_back(binding);
		for (auto const& member : type.members) {
			vk::VertexInputAttributeDescription attribute;
			attribute.binding = u32(info.bindStart + bindDelta);
			attribute.format = member.format;
			attribute.offset = (u32)member.offset;
			attribute.location = u32(info.locationStart + locationDelta++);
			ret.attributes.push_back(attribute);
		}
		++bindDelta;
	}
	return ret;
}

VertexInputInfo RenderContext::vertexInput(QuickVertexInput const& info) {
	VertexInputInfo ret;
	vk::VertexInputBindingDescription binding;
	binding.binding = info.binding;
	binding.stride = (u32)info.size;
	binding.inputRate = vk::VertexInputRate::eVertex;
	ret.bindings.push_back(binding);
	u32 location = 0;
	for (auto const& [format, offset] : info.attributes) {
		vk::VertexInputAttributeDescription attribute;
		attribute.binding = info.binding;
		attribute.format = format;
		attribute.offset = (u32)offset;
		attribute.location = location++;
		ret.attributes.push_back(attribute);
	}
	return ret;
}

RenderContext::RenderContext(not_null<VRAM*> vram, std::optional<VSync> vsync, Extent2D fbSize, Buffering buffering)
	: m_surface(vram), m_buffering(buffering), m_vram(vram) {
	m_storage.pipelineCache = makeDeferred<vk::PipelineCache>(m_vram->m_device);
	m_surface.makeSwapchain(fbSize, vsync);
	validateBuffering({(u8)m_surface.imageCount()}, m_buffering);
	DeferQueue::defaultDefer = m_buffering;
}

vk::UniqueRenderPass RenderContext::makeRenderPass(vk::Device device, Attachment colour, Attachment depth, vAP<vk::SubpassDependency> deps) {
	vk::AttachmentDescription attachments[2];
	u32 attachmentCount = 1;
	vk::AttachmentReference colourAttachment, depthAttachment;
	attachments[0].format = colour.format;
	attachments[0].samples = vk::SampleCountFlagBits::e1;
	attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
	attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
	attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[0].initialLayout = colour.layouts.first;
	attachments[0].finalLayout = colour.layouts.second;
	colourAttachment.attachment = 0;
	colourAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
	if (depth.format != vk::Format()) {
		attachments[1].format = depth.format;
		attachments[1].samples = vk::SampleCountFlagBits::e1;
		attachments[1].loadOp = vk::AttachmentLoadOp::eClear;
		attachments[1].storeOp = vk::AttachmentStoreOp::eDontCare;
		attachments[1].stencilLoadOp = vk::AttachmentLoadOp::eClear;
		attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachments[1].initialLayout = depth.layouts.first;
		attachments[1].finalLayout = depth.layouts.second;
		depthAttachment.attachment = 1;
		depthAttachment.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		attachmentCount = 2;
	}
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachment;
	subpass.pDepthStencilAttachment = depth.format == vk::Format() ? nullptr : &depthAttachment;
	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = attachmentCount;
	createInfo.pAttachments = attachments;
	createInfo.subpassCount = 1U;
	createInfo.pSubpasses = &subpass;
	createInfo.dependencyCount = deps.size();
	createInfo.pDependencies = deps.data();
	return device.createRenderPassUnique(createInfo);
}

Pipeline RenderContext::makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo info) {
	if (info.renderPass == vk::RenderPass()) { info.renderPass = m_storage.renderer->renderPass3D(); }
	info.buffering = m_storage.renderer->buffering();
	info.cache = m_storage.pipelineCache;
	return Pipeline(m_vram, shader, std::move(info), id);
}

bool RenderContext::ready(Extent2D framebufferSize) {
	if (m_storage.reconstruct.trigger) {
		if (m_surface.makeSwapchain(framebufferSize, m_storage.reconstruct.vsync)) {
			m_storage.renderer->refresh();
			m_storage.reconstruct = {};
		} else {
			return false;
		}
	}
	return true;
}

std::optional<RenderTarget> RenderContext::beginFrame(Extent2D fbSize) {
	m_storage.fbSize = fbSize;
	if (m_storage.acquire) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return std::nullopt;
	}
	auto& sync = m_sync.get();
	m_vram->update();
	m_vram->m_device->waitFor(sync.drawn);
	m_vram->m_device->resetFence(sync.drawn);
	m_vram->m_device->resetCommandPool(sync.pool);
	m_vram->m_device->decrementDeferred();
	if (auto acquire = m_surface.acquireNextImage(fbSize, m_sync.get().draw)) {
		m_storage.acquire = *acquire;
		// TODO: Pass acquired image to renderer, return render target
		sync.cb.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		m_storage.drawing = m_storage.renderer->beginFrame();
	}
	return m_storage.drawing;
}

std::optional<CommandBuffer> RenderContext::beginDraw(ScreenView const& view, RGBA clear, ClearDepth depth) {
	if (!m_storage.drawing) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return std::nullopt;
	}
	return m_storage.renderer->beginDraw(*m_storage.drawing, view, clear, depth);
}

bool RenderContext::endDraw() {
	if (!m_storage.drawing) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	m_storage.renderer->endDraw(*m_storage.drawing);
	return true;
}

bool RenderContext::endFrame() {
	if (!m_storage.drawing) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	m_storage.drawing.reset();
	m_storage.renderer->endFrame();
	return true;
}

bool RenderContext::submitFrame() {
	if (!m_storage.acquire) {
		g_log.log(lvl::warn, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	m_storage.acquire.reset();
	auto const& buf = m_sync.get();
	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags const waitStages = vPSFB::eTopOfPipe;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &buf.draw.m_t;
	submitInfo.pWaitDstStageMask = &waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buf.cb.m_cb;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &buf.present.m_t;
	return m_surface.present(m_storage.fbSize, *m_storage.acquire, buf.present);
}

void RenderContext::reconstruct(std::optional<graphics::VSync> vsync) {
	m_storage.reconstruct.trigger = true;
	m_storage.reconstruct.vsync = vsync;
}

vk::Viewport RenderContext::viewport(Extent2D extent, ScreenView const& view, glm::vec2 depth) const noexcept {
	return m_storage.renderer->viewport(Surface::valid(extent) ? extent : this->extent(), view, depth);
}

vk::Rect2D RenderContext::scissor(Extent2D extent, ScreenView const& view) const noexcept {
	return m_storage.renderer->scissor(Surface::valid(extent) ? extent : this->extent(), view);
}
} // namespace foo
} // namespace le::graphics
