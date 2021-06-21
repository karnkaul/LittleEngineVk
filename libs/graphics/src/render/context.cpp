#include <map>
#include <stdexcept>
#include <core/log.hpp>
#include <core/maths.hpp>
#include <glm/gtx/transform.hpp>
#include <graphics/common.hpp>
#include <graphics/context/defer_queue.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/context.hpp>

namespace le::graphics {
namespace {
void validateBuffering([[maybe_unused]] Buffering images, Buffering buffering) {
	ensure(images > 1_B, "Insufficient swapchain images");
	ensure(buffering > 0_B, "Insufficient buffering");
	if ((s16)buffering.value - (s16)images.value > 1) { g_log.log(lvl::warning, 0, "[{}] Buffering significantly more than swapchain image count", g_name); }
	if (buffering < 2_B) { g_log.log(lvl::warning, 0, "[{}] Buffering less than double; expect hitches", g_name); }
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

RenderContext::RenderContext(not_null<Swapchain*> swapchain, std::unique_ptr<ARenderer>&& renderer)
	: m_pool(swapchain->m_device, vk::CommandPoolCreateFlagBits::eTransient), m_swapchain(swapchain), m_device(swapchain->m_device) {
	m_storage.renderer = std::move(renderer);
	m_storage.status = Status::eWaiting;
	validateBuffering(m_swapchain->buffering(), m_storage.renderer->buffering());
	DeferQueue::defaultDefer = m_storage.renderer->buffering();
	m_storage.pipelineCache = makeDeferred<vk::PipelineCache>(m_device);
}

Pipeline RenderContext::makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo info) {
	if (info.renderPass == vk::RenderPass()) { info.renderPass = m_storage.renderer->renderPass3D(); }
	info.buffering = m_storage.renderer->buffering();
	info.cache = *m_storage.pipelineCache;
	return Pipeline(m_swapchain->m_vram, shader, std::move(info), id);
}

bool RenderContext::ready(glm::ivec2 framebufferSize) {
	if (m_swapchain->flags().any(Swapchain::Flags(Swapchain::Flag::eOutOfDate) | Swapchain::Flag::ePaused)) {
		if (m_swapchain->reconstruct(framebufferSize)) {
			m_storage.renderer->refresh();
			m_storage.status = Status::eWaiting;
		}
		return false;
	}
	return true;
}

bool RenderContext::waitForFrame() {
	if (!check(Status::eReady)) {
		if (m_storage.status != Status::eWaiting && m_storage.status != Status::eBegun) {
			g_log.log(lvl::warning, 1, "[{}] Invalid RenderContext status", g_name);
			return false;
		}
		m_storage.renderer->waitForFrame();
		m_pool.update();
		set(Status::eReady);
	}
	return true;
}

std::optional<ARenderer::Draw> RenderContext::beginFrame() {
	if (!check(Status::eReady)) {
		g_log.log(lvl::warning, 1, "[{}] Invalid RenderContext status", g_name);
		return std::nullopt;
	}
	if (m_swapchain->flags().any(Swapchain::Flags(Swapchain::Flag::ePaused) | Swapchain::Flag::eOutOfDate)) { return std::nullopt; }
	if (auto ret = m_storage.renderer->beginFrame()) {
		set(Status::eBegun);
		m_storage.target = ret->target;
		return ret;
	}
	return std::nullopt;
}

bool RenderContext::beginDraw(graphics::FrameDrawer& out_drawer, RGBA clear, vk::ClearDepthStencilValue depth) {
	if (!check(Status::eBegun)) {
		g_log.log(lvl::warning, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	if (m_storage.target) {
		set(Status::eDrawing);
		m_storage.renderer->m_viewport = {m_viewport.rect, m_viewport.offset};
		m_storage.renderer->beginDraw(*m_storage.target, out_drawer, clear, depth);
		return true;
	}
	return false;
}

bool RenderContext::endDraw() {
	if (!m_storage.target || !check(Status::eDrawing)) {
		g_log.log(lvl::warning, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	m_storage.renderer->endDraw(*m_storage.target);
	return true;
}

bool RenderContext::endFrame() {
	if (!check(Status::eDrawing, Status::eBegun)) {
		g_log.log(lvl::warning, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	set(Status::eEnded);
	m_storage.target.reset();
	m_storage.renderer->endFrame();
	return true;
}

bool RenderContext::submitFrame() {
	if (!check(Status::eEnded)) {
		g_log.log(lvl::warning, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	set(Status::eWaiting);
	if (m_storage.renderer->submitFrame()) { return true; }
	return false;
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

vk::Viewport RenderContext::viewport(Extent2D extent, ScreenRect const& nRect, glm::vec2 offset, glm::vec2 depth) const noexcept {
	return m_storage.renderer->viewport(Swapchain::valid(extent) ? extent : this->extent(), nRect, offset, depth);
}

vk::Rect2D RenderContext::scissor(Extent2D extent, ScreenRect const& nRect, glm::vec2 offset) const noexcept {
	return m_storage.renderer->scissor(Swapchain::valid(extent) ? extent : this->extent(), nRect, offset);
}
} // namespace le::graphics
