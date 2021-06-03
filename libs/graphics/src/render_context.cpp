#include <map>
#include <stdexcept>
#include <core/log.hpp>
#include <core/maths.hpp>
#include <glm/gtx/transform.hpp>
#include <graphics/common.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render_context.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
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

RenderContext::RenderContext(not_null<Swapchain*> swapchain, u32 rotateCount, u32 secondaryCmdCount)
	: m_sync(swapchain->m_device, rotateCount, secondaryCmdCount), m_swapchain(swapchain), m_vram(swapchain->m_vram), m_device(swapchain->m_device) {
	m_storage.status = Status::eReady;
}

RenderContext::RenderContext(RenderContext&& rhs)
	: m_sync(std::move(rhs.m_sync)), m_storage(std::move(rhs.m_storage)), m_swapchain(rhs.m_swapchain), m_vram(rhs.m_vram), m_device(rhs.m_device) {
	rhs.m_storage.status = Status::eIdle;
}

RenderContext& RenderContext::operator=(RenderContext&& rhs) {
	if (&rhs != this) {
		m_sync = std::move(rhs.m_sync);
		m_storage = std::move(rhs.m_storage);
		m_swapchain = rhs.m_swapchain;
		m_vram = rhs.m_vram;
		m_device = rhs.m_device;
		rhs.m_storage.status = Status::eIdle;
	}
	return *this;
}

bool RenderContext::waitForFrame() {
	if (!m_vram->m_transfer.polling()) { m_vram->m_transfer.update(); }
	if (m_storage.status == Status::eReady) { return true; }
	if (m_storage.status != Status::eWaiting) {
		g_log.log(lvl::warning, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	auto& sync = m_sync.get();
	m_device->waitFor(sync.sync.drawing);
	m_device->decrementDeferred();
	m_storage.status = Status::eReady;
	return true;
}

std::optional<RenderContext::Frame> RenderContext::beginFrame(CommandBuffer::PassInfo const& info) {
	if (m_storage.status != Status::eReady) {
		g_log.log(lvl::warning, 1, "[{}] Invalid RenderContext status", g_name);
		return std::nullopt;
	}
	if (m_swapchain->flags().any(Swapchain::Flags(Swapchain::Flag::ePaused) | Swapchain::Flag::eOutOfDate)) { return std::nullopt; }
	FrameSync& sync = m_sync.get();
	auto target = m_swapchain->acquireNextImage(sync.sync);
	if (!target) { return std::nullopt; }
	m_storage.status = Status::eDrawing;
	m_device->destroy(sync.framebuffer);
	sync.framebuffer = m_device->makeFramebuffer(m_swapchain->renderPass(), target->attachments(), target->extent);
	if (!sync.primary.commandBuffer.begin(m_swapchain->renderPass(), sync.framebuffer, target->extent, info)) {
		ENSURE(false, "Failed to begin recording command buffer");
		m_storage.status = Status::eReady;
		m_device->destroy(sync.framebuffer, sync.sync.drawReady);
		sync.sync.drawReady = m_device->makeSemaphore(); // sync.drawReady will be signalled by acquireNextImage and cannot be reused
		return std::nullopt;
	}
	return Frame{*target, sync.primary.commandBuffer};
}

bool RenderContext::endFrame() {
	if (m_storage.status != Status::eDrawing) {
		g_log.log(lvl::warning, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	FrameSync& sync = m_sync.get();
	sync.primary.commandBuffer.end();
	vk::SubmitInfo submitInfo;
	vk::PipelineStageFlags const waitStages = vk::PipelineStageFlagBits::eTopOfPipe;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &sync.sync.drawReady;
	submitInfo.pWaitDstStageMask = &waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &sync.primary.commandBuffer.m_cb;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &sync.sync.presentReady;
	m_device->device().resetFences(sync.sync.drawing);
	m_device->queues().submit(QType::eGraphics, submitInfo, sync.sync.drawing, false);
	m_storage.status = Status::eWaiting;
	auto present = m_swapchain->present(sync.sync);
	if (!present) { return false; }
	m_sync.swap();
	return true;
}

bool RenderContext::ready(glm::ivec2 framebufferSize) {
	if (m_swapchain->flags().any(Swapchain::Flags(Swapchain::Flag::eOutOfDate) | Swapchain::Flag::ePaused)) {
		if (m_swapchain->reconstruct(framebufferSize)) { m_storage.status = Status::eWaiting; }
		return false;
	}
	return true;
}

Pipeline RenderContext::makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo createInfo) {
	createInfo.renderPass = m_swapchain->renderPass();
	return Pipeline(m_vram, shader, std::move(createInfo), id);
}

glm::mat4 RenderContext::preRotate() const noexcept {
	glm::mat4 ret(1.0f);
	f32 rad = 0.0f;
	auto const transform = m_swapchain->display().transform;
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

vk::Viewport RenderContext::viewport(glm::ivec2 extent, glm::vec2 depth, ScreenRect const& nRect, glm::vec2 offset) const noexcept {
	if (!Swapchain::valid(extent)) { extent = this->extent(); }
	DrawViewport view;
	glm::vec2 const e = {(f32)extent.x, (f32)extent.y};
	view.lt = nRect.lt * e + offset;
	view.rb = nRect.rb * e + offset;
	view.depth = depth;
	return utils::viewport(view);
}

vk::Rect2D RenderContext::scissor(glm::ivec2 extent, ScreenRect const& nRect, glm::vec2 offset) const noexcept {
	if (!Swapchain::valid(extent)) { extent = this->extent(); }
	DrawScissor scissor;
	glm::vec2 const e = {(f32)extent.x, (f32)extent.y};
	scissor.lt = nRect.lt * e + offset;
	scissor.rb = nRect.rb * e + offset;
	return utils::scissor(scissor);
}
} // namespace le::graphics
