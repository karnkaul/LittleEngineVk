#include <core/log.hpp>
#include <core/log_channel.hpp>
#include <core/maths.hpp>
#include <core/utils/expect.hpp>
#include <glm/gtx/transform.hpp>
#include <graphics/common.hpp>
#include <graphics/context/defer_queue.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/context.hpp>
#include <graphics/utils/utils.hpp>
#include <map>
#include <stdexcept>

namespace le::graphics {
namespace {
void validateBuffering([[maybe_unused]] Buffering images, Buffering buffering) {
	ENSURE(images > 1_B, "Insufficient swapchain images");
	ENSURE(buffering > 0_B, "Insufficient buffering");
	if ((s16)buffering.value - (s16)images.value > 1) { logW(LC_LibUser, "[{}] Buffering significantly more than swapchain image count", g_name); }
	if (buffering < 2_B) { logW(LC_LibUser, "[{}] Buffering less than double; expect hitches", g_name); }
}

std::unique_ptr<Renderer> makeRenderer(VRAM* vram, Surface::Format const& format, BlitFlags bf, Buffering buffering) {
	Renderer::CreateInfo rci(vram, format);
	rci.buffering = buffering;
	rci.surfaceBlitFlags = bf;
	rci.target = Renderer::Target::eOffScreen;
	return std::make_unique<Renderer>(rci);
}
} // namespace

VertexInputInfo VertexInfoFactory<Vertex>::operator()(u32 binding) const {
	QuickVertexInput qvi;
	qvi.binding = binding;
	qvi.size = sizeof(Vertex);
	qvi.attributes = {{vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
					  {vk::Format::eR32G32B32Sfloat, offsetof(Vertex, colour)},
					  {vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
					  {vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)}};
	return RenderContext::vertexInput(qvi);
}

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

RenderContext::RenderContext(not_null<VRAM*> vram, GetSpirV&& gs, std::optional<VSync> vsync, Extent2D fbSize, Buffering bf)
	: m_surface(vram, fbSize, vsync), m_pipelineFactory(vram, std::move(gs), bf), m_vram(vram),
	  m_renderer(makeRenderer(m_vram, m_surface.format(), m_surface.blitFlags(), bf)), m_buffering(bf) {
	m_pipelineCache = m_pipelineCache.make(m_vram->m_device->makePipelineCache(), m_vram->m_device);
	validateBuffering({(u8)m_surface.imageCount()}, m_buffering);
	DeferQueue::defaultDefer = m_buffering;
	for (Buffering i = {}; i < m_buffering; ++i.value) { m_syncs.push(Sync::make(m_vram->m_device)); }
}

std::unique_ptr<Renderer> RenderContext::defaultRenderer() { return makeRenderer(m_vram, m_surface.format(), m_surface.blitFlags(), m_buffering); }

void RenderContext::setRenderer(std::unique_ptr<Renderer>&& renderer) noexcept {
	m_vram->m_device->waitIdle();
	m_renderer = std::move(renderer);
}

void RenderContext::waitForFrame() { m_vram->m_device->waitFor(m_syncs.get().drawn); }

std::optional<RenderPass> RenderContext::beginMainPass(RenderBegin const& rb, Extent2D fbSize) {
	m_vram->m_device->decrementDeferred();
	if (fbSize.x == 0 || fbSize.y == 0) { return std::nullopt; }
	auto& sync = m_syncs.get();
	if (auto acquired = m_surface.acquireNextImage(fbSize, sync.draw)) {
		m_acquired = *acquired;
		m_vram->m_device->resetFence(sync.drawn, true);
		return m_renderer->beginMainPass(m_pipelineFactory, m_acquired->image, rb);
	}
	return std::nullopt;
}

bool RenderContext::endMainPass(RenderPass& out_rp, Extent2D fbSize) {
	bool ret{};
	if (m_acquired) {
		auto cb = m_renderer->endMainPass(out_rp);
		ret = submit(cb, *m_acquired, fbSize);
		if (ret) { m_previousFrame = m_acquired->image; }
		m_acquired.reset();
	}
	m_syncs.next();
	return ret;
}

bool RenderContext::recreateSwapchain(Extent2D fbSize, std::optional<VSync> vsync) {
	return m_surface.makeSwapchain(fbSize, vsync.value_or(m_surface.format().vsync));
}

bool RenderContext::submit(vk::CommandBuffer cb, Acquire const& acquired, Extent2D fbSize) {
	if (fbSize.x == 0 || fbSize.y == 0) { return false; }
	auto const& sync = m_syncs.get();
	auto const res = m_surface.submit(cb, {sync.draw, sync.present, sync.drawn});
	EXPECT(res == vk::Result::eSuccess);
	if (res != vk::Result::eSuccess) {
		logW(LC_LibUser, "[Graphics] Queue submit failure");
		m_previousFrame = {};
		return false;
	}
	if (m_surface.present(fbSize, acquired, sync.present)) { return true; }
	m_previousFrame = {};
	return false;
}
} // namespace le::graphics
