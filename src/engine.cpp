#include <fstream>
#include <iostream>
#include <build_version.hpp>
#include <core/io/zip_media.hpp>
#include <core/utils/data_store.hpp>
#include <engine/engine.hpp>
#include <engine/gui/view.hpp>
#include <engine/input/space.hpp>
#include <engine/scene/list_drawer.hpp>
#include <engine/utils/engine_config.hpp>
#include <engine/utils/logger.hpp>
#include <graphics/common.hpp>
#include <graphics/mesh.hpp>
#include <graphics/render/command_buffer.hpp>
#include <graphics/utils/utils.hpp>
#include <window/glue.hpp>

namespace le {
namespace {
template <typename T>
void profilerNext(T& out_profiler, Time_s total) {
	if constexpr (!std::is_same_v<T, utils::NullProfileDB>) { out_profiler.next(total); }
}

std::optional<utils::EngineConfig> load(io::Path const& path) {
	if (auto json = dj::json(); json.load(path.generic_string())) { return io::fromJson<utils::EngineConfig>(json); }
	return std::nullopt;
}

bool save(utils::EngineConfig const& config, io::Path const& path) {
	dj::json original;
	original.read(path.generic_string());
	auto overwrite = io::toJson(config);
	for (auto& [id, json] : overwrite.as<dj::map_t>()) {
		if (original.contains(id)) {
			original[id] = std::move(*json);
		} else {
			original.insert(id, std::move(*json));
		}
	}
	dj::serial_opts_t opts;
	opts.sort_keys = opts.pretty = true;
	return original.save(path.generic_string(), opts);
}
} // namespace

Engine::Boot::MakeSurface Engine::GFX::makeSurface(Window const& winst) {
	return [&winst](vk::Instance vkinst) { return window::makeSurface(vkinst, winst); };
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

Engine::Engine(CreateInfo const& info, io::Media const* custom) : m_io(info.logFile.value_or(io::Path())) {
	utils::g_log.minVerbosity = info.verbosity;
	if (custom) { m_store.resources().media(custom); }
	logI("LittleEngineVk v{} | {}", version().toString(false), time::format(time::sysTime(), "{:%a %F %T %Z}"));
	ensure(m_wm.ready(), "Window Manager not ready");
	auto winInfo = info.winInfo;
	winInfo.options.autoShow = false;
	if (auto config = load(m_configPath)) {
		logI("[Engine] Config loaded from {}", m_configPath.generic_string());
		if (config->win.size.x > 0 && config->win.size.y > 0) { winInfo.config.size = config->win.size; }
		winInfo.config.position = config->win.position;
	}
	m_win = m_wm.make(winInfo);
	m_errorHandler.deleteFile();
	if (!m_errorHandler.activeHandler()) { m_errorHandler.setActive(); }
}

Engine::~Engine() { unboot(); }

input::Driver::Out Engine::poll(bool consume) noexcept {
	if (!bootReady()) { return {}; }
	f32 const rscale = m_gfx ? m_gfx->context.renderer().renderScale() : 1.0f;
	input::Driver::In in{m_win->pollEvents(), {framebufferSize(), sceneSpace()}, rscale, &*m_win};
	auto ret = m_input.update(std::move(in), m_editor.view(), consume);
	m_inputFrame = ret.frame;
	for (auto it = m_receivers.rbegin(); it != m_receivers.rend(); ++it) {
		if ((*it)->block(ret.frame.state)) { break; }
	}
	if (m_inputFrame.state.focus == input::Focus::eGained) { m_store.update(); }
	profilerNext(m_profiler, time::diffExchg(m_lastPoll));
	return ret;
}

void Engine::update(gui::ViewStack& out_stack) { out_stack.update(m_inputFrame); }

void Engine::pushReceiver(not_null<input::Receiver*> context) { context->pushSelf(m_receivers); }

bool Engine::drawReady() {
	if (bootReady() && m_gfx) {
		if (!m_gfx->context.ready(m_win->framebufferSize())) { return false; }
		return true;
	}
	return false;
}

bool Engine::nextFrame(graphics::RenderTarget* out) {
	auto pr_ = m_profiler.profile("eng::nextFrame");
	if (bootReady() && !m_drawing && drawReady() && m_gfx->context.waitForFrame()) {
		if (auto ret = m_gfx->context.beginFrame()) {
			updateStats();
			if constexpr (levk_imgui) {
				[[maybe_unused]] bool const b = m_gfx->imgui.beginFrame();
				ensure(b, "Failed to begin DearImGui frame");
				m_view = m_editor.update(m_inputFrame);
			}
			m_drawing = *ret;
			if (out) { *out = *ret; }
			return true;
		}
	}
	return false;
}

bool Engine::draw(ListDrawer& drawer, RGBA clear, ClearDepth depth) {
	auto pr_ = m_profiler.profile("eng::draw");
	if (auto cb = beginDraw(clear, depth)) {
		drawer.draw(*cb);
		return endDraw(*cb);
	}
	return false;
}

bool Engine::unboot() noexcept {
	if (m_gfx) {
		saveConfig();
		m_store.clear();
		Services::untrack<Context, VRAM, AssetStore, Profiler>();
		m_gfx.reset();
		io::ZIPMedia::fsDeinit();
		return true;
	}
	return false;
}

Extent2D Engine::framebufferSize() const noexcept {
	if (m_gfx) { return m_gfx->context.extent(); }
	if (m_win) { return m_win->framebufferSize(); }
	return {};
}

Extent2D Engine::windowSize() const noexcept { return m_win->windowSize(); }

void Engine::updateStats() {
	m_stats.update();
	m_stats.stats.gfx.bytes.buffers = m_gfx->boot.vram.bytes(graphics::Resource::Kind::eBuffer);
	m_stats.stats.gfx.bytes.images = m_gfx->boot.vram.bytes(graphics::Resource::Kind::eImage);
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

Engine::Boot::CreateInfo Engine::adjust(Boot::CreateInfo const& info) {
	auto ret = info;
	ret.instance.extensions = window::instanceExtensions(*m_win);
	if (auto gpuOverride = DataObject<std::size_t>("gpuOverride")) { ret.device.pickOverride = *gpuOverride; }
	return ret;
}

void Engine::bootImpl() {
	Services::track<Context, VRAM, AssetStore, Profiler>(&m_gfx->context, &m_gfx->boot.vram, &m_store, &m_profiler);
	DearImGui::CreateInfo dici(m_gfx->context.renderer().renderPassUI());
	dici.correctStyleColours = m_gfx->context.colourCorrection() == graphics::ColourCorrection::eAuto;
	m_gfx->imgui = DearImGui(&m_gfx->boot.device, &*m_win, dici);
	addDefaultAssets();
	m_win->show();
}

void Engine::addDefaultAssets() {
	static_assert(detail::reloadable_asset_v<graphics::Texture>, "ODR violation! include asset_loaders.hpp");
	static_assert(!detail::reloadable_asset_v<int>, "ODR violation! include asset_loaders.hpp");
	auto sampler = m_store.add("samplers/default", graphics::Sampler{&gfx().boot.device, graphics::Sampler::info({vk::Filter::eLinear, vk::Filter::eLinear})});
	/*Textures*/ {
		graphics::Texture::CreateInfo tci;
		tci.sampler = sampler->sampler();
		tci.data = graphics::utils::bitmap({0xff0000ff}, 1);
		graphics::Texture texture(&gfx().boot.vram);
		if (texture.construct(tci)) { m_store.add("textures/red", std::move(texture)); }
		tci.data = graphics::utils::bitmap({0x000000ff}, 1);
		if (texture.construct(tci)) { m_store.add("textures/black", std::move(texture)); }
		tci.data = graphics::utils::bitmap({0xffffffff}, 1);
		if (texture.construct(tci)) { m_store.add("textures/white", std::move(texture)); }
		tci.data = graphics::utils::bitmap({0x0}, 1);
		if (texture.construct(tci)) { m_store.add("textures/blank", std::move(texture)); }
		tci.data = graphics::Texture::unitCubemap(colours::transparent);
		if (texture.construct(tci)) { m_store.add("cubemaps/blank", std::move(texture)); }
	}
	/* meshes */ {
		auto cube = m_store.add<graphics::Mesh>("meshes/cube", graphics::Mesh(&gfx().boot.vram));
		cube->construct(graphics::makeCube());
		auto cone = m_store.add<graphics::Mesh>("meshes/cone", graphics::Mesh(&gfx().boot.vram));
		cone->construct(graphics::makeCone());
		auto wf_cube = m_store.add<graphics::Mesh>("wireframes/cube", graphics::Mesh(&m_gfx->boot.vram));
		wf_cube->construct(graphics::makeCube(1.0f, {}, graphics::Topology::eLineList));
	}
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

void Engine::saveConfig() const {
	utils::EngineConfig config;
	config.win.position = m_win->position();
	config.win.size = m_win->windowSize();
	config.win.maximized = m_win->maximized();
	if (save(config, m_configPath)) { logI("[Engine] Config saved to {}", m_configPath.generic_string()); }
}
} // namespace le
