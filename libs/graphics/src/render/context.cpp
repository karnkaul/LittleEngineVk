#include <map>
#include <stdexcept>
#include <core/log.hpp>
#include <core/maths.hpp>
#include <glm/gtx/transform.hpp>
#include <graphics/common.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/context.hpp>
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

RenderContext::RenderContext(not_null<Swapchain*> swapchain, std::unique_ptr<ARenderer> renderer) : m_swapchain(swapchain), m_device(swapchain->m_device) {
	m_storage.renderer = std::move(renderer);
	m_storage.status = Status::eWaiting;
}

RenderContext::RenderContext(RenderContext&& rhs) : m_storage(std::move(rhs.m_storage)), m_swapchain(rhs.m_swapchain), m_device(rhs.m_device) {
	rhs.m_storage.status = Status::eIdle;
}

RenderContext& RenderContext::operator=(RenderContext&& rhs) {
	if (&rhs != this) {
		m_storage = std::move(rhs.m_storage);
		m_swapchain = rhs.m_swapchain;
		m_device = rhs.m_device;
		rhs.m_storage.status = Status::eIdle;
	}
	return *this;
}

bool RenderContext::waitForFrame() {
	if (m_storage.status != Status::eReady) {
		if (m_storage.status != Status::eWaiting) {
			g_log.log(lvl::warning, 1, "[{}] Invalid RenderContext status", g_name);
			return false;
		}
		m_storage.renderer->waitForFrame();
		m_storage.status = Status::eReady;
	}
	return true;
}

std::optional<ARenderer::Draw> RenderContext::beginFrame(CommandBuffer::PassInfo const& info) {
	if (m_storage.status != Status::eReady) {
		g_log.log(lvl::warning, 1, "[{}] Invalid RenderContext status", g_name);
		return std::nullopt;
	}
	if (m_swapchain->flags().any(Swapchain::Flags(Swapchain::Flag::ePaused) | Swapchain::Flag::eOutOfDate)) { return std::nullopt; }
	if (auto ret = m_storage.renderer->beginFrame(info)) {
		m_storage.status = Status::eDrawing;
		return ret;
	}
	return std::nullopt;
}

bool RenderContext::endFrame() {
	if (m_storage.status != Status::eDrawing) {
		g_log.log(lvl::warning, 1, "[{}] Invalid RenderContext status", g_name);
		return false;
	}
	if (m_storage.renderer->endFrame()) {
		m_storage.status = Status::eWaiting;
		return true;
	}
	return false;
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

Pipeline RenderContext::makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo createInfo) {
	if (createInfo.renderPass == vk::RenderPass()) { createInfo.renderPass = m_storage.renderer->renderPasses().front(); }
	return Pipeline(m_swapchain->m_vram, shader, std::move(createInfo), id);
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
