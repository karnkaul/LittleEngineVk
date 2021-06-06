#include <iostream>
#include <build_version.hpp>
#include <engine/config.hpp>
#include <engine/engine.hpp>
#include <engine/gui/view.hpp>
#include <graphics/common.hpp>
#include <graphics/context/command_buffer.hpp>
#include <graphics/mesh.hpp>
#include <window/android_instance.hpp>
#include <window/desktop_instance.hpp>

namespace le {
Engine::GFX::GFX(not_null<Window const*> winst, Boot::CreateInfo const& bci)
	: boot(bci, makeSurface(*winst), winst->framebufferSize()), context(&boot.swapchain) {
#if defined(LEVK_DESKTOP)
	DearImGui::CreateInfo dici(boot.swapchain.renderPass());
	dici.correctStyleColours = context.colourCorrection() == graphics::ColourCorrection::eAuto;
	imgui = DearImGui(&boot.device, static_cast<window::DesktopInstance const*>(winst.get()), dici);
#endif
}

Engine::Boot::MakeSurface Engine::GFX::makeSurface(Window const& winst) {
	return [&winst](vk::Instance vkinst) {
		vk::SurfaceKHR ret;
		winst.vkCreateSurface(&vkinst, &ret);
		return ret;
	};
}

Engine::DrawFrame::DrawFrame(not_null<Engine*> engine, Context::Frame&& frame) noexcept : frame(std::move(frame)), engine(engine) {}

Engine::DrawFrame::DrawFrame(DrawFrame&& rhs) noexcept : frame(std::exchange(rhs.frame, Context::Frame())), engine(rhs.engine) {}

Engine::DrawFrame& Engine::DrawFrame::operator=(DrawFrame&& rhs) noexcept {
	if (&rhs != this) {
		if (frame.primary.valid()) { engine->endDraw(frame); }
		frame = std::exchange(rhs.frame, Context::Frame());
		engine = rhs.engine;
	}
	return *this;
}

Engine::DrawFrame::~DrawFrame() {
	if (frame.primary.valid()) { engine->endDraw(frame); }
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
	conf::g_log.minVerbosity = info.verbosity;
	logI("LittleEngineVk v{} | {}", version().toString(false), time::format(time::sysTime(), "{:%a %F %T %Z}"));
}

input::Driver::Out Engine::poll(bool consume) noexcept {
	auto const extent = m_gfx ? m_gfx->context.extent() : glm::ivec2(0);
	auto ret = m_input.update(m_win->pollEvents(), m_editor.view(), extent, consume, m_desktop);
	m_inputState = ret.state;
	for (auto it = m_receivers.rbegin(); it != m_receivers.rend(); ++it) {
		if ((*it)->block(ret.state)) { break; }
	}
	return ret;
}

void Engine::update(gui::ViewStack& out_stack) {
	glm::vec2 wSize = {};
#if defined(LEVK_DESKTOP)
	ENSURE(m_win->isDesktop(), "Invariant violated");
	wSize = m_desktop->windowSize();
#endif
	out_stack.update(m_inputState, m_editor.view(), framebufferSize(), wSize);
}

void Engine::pushReceiver(not_null<input::Receiver*> context) { context->m_inputHandle = m_receivers.push(context); }

bool Engine::beginFrame(bool waitDrawReady) {
	updateStats();
	if (m_gfx) {
		auto const size = m_win->framebufferSize();
		if constexpr (levk_desktopOS) {
			if (m_recreateInterval > 0ms) {
				if (m_fb.size.x == 0 || m_fb.size.y == 0) {
					m_fb.size = size;
					m_fb.resized = {};
				} else if (size != m_fb.size) {
					m_fb.size = size;
					m_fb.resized = time::now();
					return false;
				}
				if (m_fb.resized != time::Point{} && time::diff(m_fb.resized) < m_recreateInterval) { return false; }
			}
		}
		if (!m_gfx->context.ready(size)) { return false; }
		if constexpr (levk_imgui) {
			[[maybe_unused]] bool const b = m_gfx->imgui.beginFrame();
			ENSURE(b, "Failed to begin DearImGui frame");
		}
		if (waitDrawReady) { return drawReady(); }
		return true;
	}
	return false;
}

bool Engine::drawReady() {
	if (m_gfx) { return m_gfx->context.waitForFrame(); }
	return false;
}

std::optional<Engine::Context::Frame> Engine::beginDraw(RGBA clear, vk::ClearDepthStencilValue depth) {
	if (m_gfx) {
		if constexpr (levk_imgui) {
			if (m_gfx->imgui.state() == DearImGui::State::eBegin) {
				ENSURE(m_desktop, "Invariant violated");
				m_editor.update(*m_desktop, m_inputState);
			}
		}
		auto const cl = clear.toVec4();
		vk::ClearColorValue const c = std::array{cl.x, cl.y, cl.z, cl.w};
		graphics::CommandBuffer::PassInfo const pass{{c, depth}};
		return m_gfx->context.beginFrame(pass);
	}
	return std::nullopt;
}

std::optional<Engine::DrawFrame> Engine::drawFrame(RGBA clear, vk::ClearDepthStencilValue depth) {
	if (auto frame = beginDraw(clear, depth)) { return DrawFrame(this, std::move(*frame)); }
	return std::nullopt;
}

bool Engine::endDraw(Context::Frame const& frame) {
	if (m_gfx) {
		if constexpr (levk_imgui) {
			m_gfx->imgui.endFrame();
			m_gfx->imgui.renderDrawData(frame.primary);
		}
		return m_gfx->context.endFrame();
	}
	return false;
}

bool Engine::boot(Boot::CreateInfo boot) {
	if (!m_gfx) {
		if (s_options.gpuOverride) { boot.device.pickOverride = s_options.gpuOverride; }
		m_gfx.emplace(m_win.get(), boot);
		return true;
	}
	return false;
}

bool Engine::unboot() noexcept {
	if (m_gfx) {
		m_gfx.reset();
		return true;
	}
	return false;
}

glm::ivec2 Engine::framebufferSize() const noexcept { return m_gfx ? m_gfx->context.extent() : m_win->framebufferSize(); }

vk::Viewport Engine::viewport(Viewport const& view, glm::vec2 depth) const noexcept {
	if (!m_gfx) { return {}; }
	Viewport const vp = m_editor.view() * view;
	return m_gfx->context.viewport(m_gfx->context.extent(), depth, vp.rect(), vp.topLeft.offset);
}

vk::Rect2D Engine::scissor(Viewport const& view) const noexcept {
	if (!m_gfx) { return {}; }
	Viewport const vp = m_editor.view() * view;
	return m_gfx->context.scissor(m_gfx->context.extent(), vp.rect(), vp.topLeft.offset);
}

Engine::Desktop* Engine::desktop() const noexcept { return m_desktop; }

void Engine::updateStats() {
	++m_stats.frame.count;
	++s_stats.frame.count;
	if (m_stats.frame.stamp == time::Point()) {
		m_stats.frame.stamp = time::now();
	} else {
		s_stats.frame.ft = time::diffExchg(m_stats.frame.stamp);
		m_stats.frame.elapsed += s_stats.frame.ft;
		s_stats.upTime += s_stats.frame.ft;
	}
	if (m_stats.frame.elapsed >= 1s) {
		s_stats.frame.rate = std::exchange(m_stats.frame.count, 0);
		m_stats.frame.elapsed -= 1s;
	}
	s_stats.gfx.bytes.buffers = m_gfx->boot.vram.bytes(graphics::Resource::Type::eBuffer);
	s_stats.gfx.bytes.images = m_gfx->boot.vram.bytes(graphics::Resource::Type::eImage);
	s_stats.gfx.drawCalls = graphics::CommandBuffer::s_drawCalls.load();
	s_stats.gfx.triCount = graphics::Mesh::s_trisDrawn.load();
	graphics::CommandBuffer::s_drawCalls.store(0);
	graphics::Mesh::s_trisDrawn.store(0);
}
} // namespace le
