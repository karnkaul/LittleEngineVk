#include <iostream>
#include <build_version.hpp>
#include <engine/engine.hpp>
#include <engine/gui/view.hpp>
#include <engine/input/space.hpp>
#include <engine/scene/list_drawer.hpp>
#include <engine/utils/logger.hpp>
#include <graphics/common.hpp>
#include <graphics/mesh.hpp>
#include <graphics/render/command_buffer.hpp>
#include <window/android_instance.hpp>
#include <window/desktop_instance.hpp>

namespace le {
Engine::Boot::MakeSurface Engine::GFX::makeSurface(Window const& winst) {
	return [&winst](vk::Instance vkinst) {
		vk::SurfaceKHR ret;
		winst.vkCreateSurface(&vkinst, &ret);
		return ret;
	};
}

Version Engine::version() noexcept { return g_engineVersion; }

Span<graphics::PhysicalDevice const> Engine::availableDevices() {
	auto const verb = graphics::g_log.minVerbosity;
	if (s_devices.empty()) {
		graphics::g_log.minVerbosity = LibLogger::Verbosity::eEndUser;
		graphics::Instance inst(graphics::Instance::CreateInfo{});
		s_devices = inst.availableDevices(graphics::Device::requiredExtensions);
	}
	graphics::g_log.minVerbosity = verb;
	return s_devices;
}

Engine::Engine(not_null<Window*> winInst, CreateInfo const& info) : m_win(winInst), m_io(info.logFile.value_or(io::Path())) {
#if defined(LEVK_DESKTOP)
	m_desktop = static_cast<Desktop*>(winInst.get());
#endif
	utils::g_log.minVerbosity = info.verbosity;
	logI("LittleEngineVk v{} | {}", version().toString(false), time::format(time::sysTime(), "{:%a %F %T %Z}"));
}

Engine::~Engine() { unboot(); }

input::Driver::Out Engine::poll(bool consume) noexcept {
	f32 const rscale = m_gfx ? m_gfx->context.renderer().renderScale() : 1.0f;
	input::Driver::In in{m_win->pollEvents(), {framebufferSize(), sceneSpace()}, rscale, m_desktop};
	auto ret = m_input.update(std::move(in), m_editor.view(), consume);
	m_inputFrame = ret.frame;
	for (auto it = m_receivers.rbegin(); it != m_receivers.rend(); ++it) {
		if ((*it)->block(ret.frame.state)) { break; }
	}
	return ret;
}

void Engine::update(gui::ViewStack& out_stack) { out_stack.update(m_inputFrame); }

void Engine::pushReceiver(not_null<input::Receiver*> context) { context->m_inputHandle = m_receivers.push(context); }

bool Engine::drawReady() {
	if (m_gfx) {
		if (!m_gfx->context.ready(m_win->framebufferSize())) { return false; }
		return true;
	}
	return false;
}

bool Engine::nextFrame(graphics::RenderTarget* out) {
	if (!m_drawing && drawReady() && m_gfx->context.waitForFrame()) {
		if (auto ret = m_gfx->context.beginFrame()) {
			updateStats();
			if constexpr (levk_imgui) {
				[[maybe_unused]] bool const b = m_gfx->imgui.beginFrame();
				ensure(b, "Failed to begin DearImGui frame");
				ensure(m_desktop, "Invariant violated");
				m_view = m_editor.update(*m_desktop, m_gfx->context.renderer(), m_inputFrame);
			}
			m_drawing = *ret;
			if (out) { *out = *ret; }
			return true;
		}
	}
	return false;
}

bool Engine::draw(ListDrawer& drawer, RGBA clear, ClearDepth depth) {
	if (auto cb = beginDraw(clear, depth)) {
		drawer.draw(*cb);
		return endDraw(*cb);
	}
	return false;
}

bool Engine::unboot() noexcept {
	if (m_gfx) {
		Services::untrack<Context, VRAM>();
		m_gfx.reset();
		return true;
	}
	return false;
}

Extent2D Engine::framebufferSize() const noexcept {
	if (m_gfx) { return m_gfx->context.extent(); }
	return m_win->framebufferSize();
}

Extent2D Engine::windowSize() const noexcept {
#if defined(LEVK_DESKTOP)
	return m_desktop->windowSize();
#else
	return m_gfx ? m_gfx->context.extent() : Extent2D(0);
#endif
}

void Engine::updateStats() {
	m_stats.update();
	m_stats.stats.gfx.bytes.buffers = m_gfx->boot.vram.bytes(graphics::Resource::Type::eBuffer);
	m_stats.stats.gfx.bytes.images = m_gfx->boot.vram.bytes(graphics::Resource::Type::eImage);
	m_stats.stats.gfx.drawCalls = graphics::CommandBuffer::s_drawCalls.load();
	m_stats.stats.gfx.triCount = graphics::Mesh::s_trisDrawn.load();
	m_stats.stats.gfx.extents.window = windowSize();
	if (m_gfx) {
		m_stats.stats.gfx.extents.swapchain = m_gfx->context.extent();
		m_stats.stats.gfx.extents.renderer = ARenderer::scaleExtent(m_stats.stats.gfx.extents.swapchain, m_gfx->context.renderer().renderScale());
	}
	graphics::CommandBuffer::s_drawCalls.store(0);
	graphics::Mesh::s_trisDrawn.store(0);
}

void Engine::bootImpl() {
	Services::track<Context, VRAM>(&m_gfx->context, &m_gfx->boot.vram);
#if defined(LEVK_DESKTOP)
	DearImGui::CreateInfo dici(m_gfx->context.renderer().renderPassUI());
	dici.correctStyleColours = m_gfx->context.colourCorrection() == graphics::ColourCorrection::eAuto;
	m_gfx->imgui = DearImGui(&m_gfx->boot.device, m_desktop, dici);
#endif
}

std::optional<graphics::CommandBuffer> Engine::beginDraw(RGBA clear, ClearDepth depth) {
	m_drawing = {};
	if (auto cb = m_gfx->context.beginDraw(m_view, clear, depth)) { return cb; }
	m_gfx->context.endFrame();
	return std::nullopt;
}

bool Engine::endDraw(graphics::CommandBuffer cb) {
	if constexpr (levk_imgui) {
		m_gfx->imgui.endFrame();
		m_gfx->imgui.renderDrawData(cb);
	}
	m_gfx->context.endDraw();
	m_gfx->context.endFrame();
	return m_gfx->context.submitFrame();
}
} // namespace le
