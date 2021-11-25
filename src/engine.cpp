#include <build_version.hpp>
#include <core/io.hpp>
#include <core/io/zip_media.hpp>
#include <core/utils/data_store.hpp>
#include <core/utils/error.hpp>
#include <engine/assets/asset_loaders_store.hpp>
#include <engine/editor/editor.hpp>
#include <engine/engine.hpp>
#include <engine/gui/view.hpp>
#include <engine/input/driver.hpp>
#include <engine/input/receiver.hpp>
#include <engine/scene/scene_registry.hpp>
#include <engine/utils/engine_config.hpp>
#include <engine/utils/engine_stats.hpp>
#include <engine/utils/error_handler.hpp>
#include <graphics/utils/utils.hpp>
#include <levk_imgui/levk_imgui.hpp>
#include <window/glue.hpp>
#include <window/instance.hpp>

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

graphics::Bootstrap::MakeSurface makeSurface(window::Instance const& winst) {
	return [&winst](vk::Instance vkinst) { return window::makeSurface(vkinst, winst); };
}

graphics::RenderContext::GetSpirV getShader(AssetStore const& store) {
	return [&store](Hash uri) {
		ENSURE(store.exists<graphics::SpirV>(uri), "Shader doesn't exist");
		return *store.find<graphics::SpirV>(uri);
	};
}
} // namespace

struct Engine::Impl {
	io::Service io;
	window::Manager wm;
	std::optional<Window> win;
	std::optional<GFX> gfx;
	AssetStore store;
	input::Driver input;
	input::ReceiverStore receivers;
	input::Frame inputFrame;
	graphics::ScreenView view;
	Profiler profiler;
	time::Point lastPoll{};
	Editor editor;
	utils::EngineStats::Counter stats;
	utils::ErrorHandler errorHandler;

	Impl(std::optional<io::Path> logPath) : io(logPath.value_or(io::Path())) {}
};

Engine::GFX::GFX(not_null<Window const*> winst, Boot::CreateInfo const& bci, AssetStore const& store, std::optional<VSync> vsync)
	: boot(bci, makeSurface(*winst)), context(&boot.vram, getShader(store), vsync, winst->framebufferSize()) {}

Version Engine::version() noexcept { return g_engineVersion; }

Span<graphics::PhysicalDevice const> Engine::availableDevices() {
	auto const verb = graphics::g_log.minVerbosity;
	if (s_devices.empty()) {
		graphics::g_log.minVerbosity = LibLogger::Verbosity::eEndUser;
		s_devices = graphics::Device::physicalDevices();
	}
	graphics::g_log.minVerbosity = verb;
	return s_devices;
}

bool Engine::drawImgui(graphics::CommandBuffer cb) {
	if constexpr (levk_imgui) {
		if (auto eng = Services::find<Engine>(); eng && eng->gfx().imgui) {
			eng->gfx().imgui->endFrame();
			eng->gfx().imgui->renderDrawData(cb);
			return true;
		}
	}
	return false;
}

Engine::Engine(CreateInfo const& info, io::Media const* custom) : m_impl(std::make_unique<Impl>(std::move(info.logFile))) {
	if (!m_impl->wm.ready()) { throw std::runtime_error("Window manager not ready"); }
	utils::g_log.minVerbosity = info.verbosity;
	if (custom) { m_impl->store.resources().media(custom); }
	logI("LittleEngineVk v{} | {}", version().toString(false), time::format(time::sysTime(), "{:%a %F %T %Z}"));
	logI("Platform: {} {} ({})", levk_arch_name, levk_OS_name, os::cpuID());
	auto winInfo = info.winInfo;
	winInfo.options.autoShow = false;
	if (auto config = load(m_configPath)) {
		logI("[Engine] Config loaded from {}", m_configPath.generic_string());
		if (config->win.size.x > 0 && config->win.size.y > 0) { winInfo.config.size = config->win.size; }
		winInfo.config.position = config->win.position;
	}
	m_impl->win = m_impl->wm.make(winInfo);
	if (!m_impl->win) { throw std::runtime_error("Failed to create window"); }
	m_impl->errorHandler.deleteFile();
	if (!m_impl->errorHandler.activeHandler()) { m_impl->errorHandler.setActive(); }
	Services::track(this);
}

Engine::~Engine() {
	unboot();
	Services::untrack(this);
}

void Engine::boot(Boot::CreateInfo info, std::optional<VSync> vsync) {
	unboot();
	info.device.instance.extensions = window::instanceExtensions(*m_impl->win);
	if (auto gpuOverride = DataObject<CustomDevice>("gpuOverride")) { info.device.customDeviceName = gpuOverride->name; }
	m_impl->gfx.emplace(&*m_impl->win, info, m_impl->store, vsync);
	auto const& surface = m_impl->gfx->context.surface();
	logI("[Engine] Swapchain image count: [{}] VSync: [{}]", surface.imageCount(), graphics::vSyncNames[surface.format().vsync]);
	logD("[Engine] Device supports lazily allocated memory: {}", m_impl->gfx->boot.device.physicalDevice().supportsLazyAllocation());
	Services::track<Context, VRAM, AssetStore, Profiler>(&m_impl->gfx->context, &m_impl->gfx->boot.vram, &m_impl->store, &m_impl->profiler);
	if constexpr (levk_imgui) { m_impl->gfx->imgui = std::make_unique<DearImGui>(&m_impl->gfx->context, &*m_impl->win); }
	addDefaultAssets();
	m_impl->win->show();
}

bool Engine::unboot() noexcept {
	if (booted()) {
		saveConfig();
		m_impl->store.clear();
		Services::untrack<Context, VRAM, AssetStore, Profiler>();
		m_impl->gfx->boot.vram.shutdown();
		m_impl->gfx.reset();
		io::ZIPMedia::fsDeinit();
		return true;
	}
	return false;
}

input::Driver::Out Engine::poll(bool consume) noexcept {
	f32 const rscale = m_impl->gfx ? m_impl->gfx->context.renderer().renderScale() : 1.0f;
	input::Driver::In in{m_impl->win->pollEvents(), {framebufferSize(), sceneSpace()}, rscale, &*m_impl->win};
	auto ret = m_impl->input.update(std::move(in), editor().view(), consume);
	m_impl->inputFrame = ret.frame;
	for (auto it = m_impl->receivers.rbegin(); it != m_impl->receivers.rend(); ++it) {
		if ((*it)->block(ret.frame.state)) { break; }
	}
	if (m_impl->inputFrame.state.focus == input::Focus::eGained) { m_impl->store.update(); }
	profilerNext(m_impl->profiler, time::diffExchg(m_impl->lastPoll));
	return ret;
}

void Engine::update(gui::ViewStack& out_stack) { out_stack.update(m_impl->inputFrame); }

void Engine::pushReceiver(not_null<input::Receiver*> context) { context->attach(m_impl->receivers); }

bool Engine::booted() const noexcept { return m_impl->gfx.has_value(); }

bool Engine::setRenderer(std::unique_ptr<Renderer>&& renderer) {
	if (booted()) {
		m_impl->gfx->imgui.reset();
		m_impl->gfx->context.setRenderer(std::move(renderer));
		if constexpr (levk_imgui) { m_impl->gfx->imgui = std::make_unique<DearImGui>(&m_impl->gfx->context, &*m_impl->win); }
		return true;
	}
	return false;
}

bool Engine::nextFrame() {
	if (booted()) {
		auto pr_ = profile("nextFrame");
		gfx().context.waitForFrame();
		updateStats();
		return true;
	}
	return false;
}

bool Engine::render(IDrawer& out_drawer, RenderBegin rb, SceneRegistry* scene) {
	if (booted()) {
		auto pr_ = profile("render");
		if constexpr (levk_imgui) {
			[[maybe_unused]] bool const imgui_begun = gfx().imgui->beginFrame();
			EXPECT(imgui_begun);
			rb.view = m_impl->view = editor().update(scene ? scene->ediScene() : edi::SceneRef(), *this);
		}
		return m_impl->gfx->context.render(out_drawer, rb, m_impl->win->framebufferSize());
	}
	return false;
}

input::Receiver::Store& Engine::receiverStore() noexcept { return m_impl->receivers; }
input::Frame const& Engine::inputFrame() const noexcept { return m_impl->inputFrame; }
AssetStore& Engine::store() const noexcept { return m_impl->store; }
glm::vec2 Engine::sceneSpace() const noexcept { return m_space(m_impl->inputFrame.space); }
window::Manager& Engine::windowManager() const noexcept { return m_impl->wm; }
bool Engine::closing() const { return window().closing(); }

Engine::GFX& Engine::gfx() const {
	ENSURE(m_impl->gfx.has_value(), "Not booted");
	return *m_impl->gfx;
}

Engine::Renderer& Engine::renderer() const { return gfx().context.renderer(); }

Editor& Engine::editor() const noexcept { return m_impl->editor; }
Engine::Stats const& Engine::stats() const noexcept { return m_impl->stats.stats; }

Engine::Window& Engine::window() const {
	ENSURE(m_impl->win.has_value(), "Not booted");
	return *m_impl->win;
}
Extent2D Engine::framebufferSize() const noexcept {
	if (m_impl->gfx) { return m_impl->gfx->context.surface().extent(); }
	return m_impl->win->framebufferSize();
}
Extent2D Engine::windowSize() const noexcept { return m_impl->win->windowSize(); }

void Engine::saveConfig() const {
	utils::EngineConfig config;
	config.win.position = m_impl->win->position();
	config.win.size = m_impl->win->windowSize();
	config.win.maximized = m_impl->win->maximized();
	if (save(config, m_configPath)) { logI("[Engine] Config saved to {}", m_configPath.generic_string()); }
}

void Engine::addDefaultAssets() {
	static_assert(detail::reloadable_asset_v<graphics::Texture>, "ODR violation! include asset_loaders.hpp");
	static_assert(!detail::reloadable_asset_v<int>, "ODR violation! include asset_loaders.hpp");
	auto sampler = store().add("samplers/default", graphics::Sampler{&gfx().boot.device, graphics::Sampler::info({vk::Filter::eLinear, vk::Filter::eLinear})});
	/*Textures*/ {
		graphics::Texture::CreateInfo tci;
		tci.sampler = sampler->sampler();
		tci.data = graphics::utils::bitmap({0xff0000ff}, 1);
		graphics::Texture texture(&gfx().boot.vram);
		if (texture.construct(tci)) { store().add("textures/red", std::move(texture)); }
		tci.data = graphics::utils::bitmap({0x000000ff}, 1);
		if (texture.construct(tci)) { store().add("textures/black", std::move(texture)); }
		tci.data = graphics::utils::bitmap({0xffffffff}, 1);
		if (texture.construct(tci)) { store().add("textures/white", std::move(texture)); }
		tci.data = graphics::utils::bitmap({0x0}, 1);
		if (texture.construct(tci)) { store().add("textures/blank", std::move(texture)); }
		tci.data = graphics::Texture::unitCubemap(colours::transparent);
		if (texture.construct(tci)) { store().add("cubemaps/blank", std::move(texture)); }
	}
	/* meshes */ {
		auto cube = store().add<graphics::Mesh>("meshes/cube", graphics::Mesh(&gfx().boot.vram));
		cube->construct(graphics::makeCube());
		auto cone = store().add<graphics::Mesh>("meshes/cone", graphics::Mesh(&gfx().boot.vram));
		cone->construct(graphics::makeCone());
		auto wf_cube = store().add<graphics::Mesh>("wireframes/cube", graphics::Mesh(&gfx().boot.vram));
		wf_cube->construct(graphics::makeCube(1.0f, {}, graphics::Topology::eLineList));
	}
}

void Engine::updateStats() {
	m_impl->stats.update();
	m_impl->stats.stats.gfx.bytes.buffers = m_impl->gfx->boot.vram.bytes(graphics::Resource::Kind::eBuffer);
	m_impl->stats.stats.gfx.bytes.images = m_impl->gfx->boot.vram.bytes(graphics::Resource::Kind::eImage);
	m_impl->stats.stats.gfx.drawCalls = graphics::CommandBuffer::s_drawCalls.load();
	m_impl->stats.stats.gfx.triCount = graphics::Mesh::s_trisDrawn.load();
	m_impl->stats.stats.gfx.extents.window = windowSize();
	if (m_impl->gfx) {
		m_impl->stats.stats.gfx.extents.swapchain = m_impl->gfx->context.surface().extent();
		m_impl->stats.stats.gfx.extents.renderer =
			Renderer::scaleExtent(m_impl->stats.stats.gfx.extents.swapchain, m_impl->gfx->context.renderer().renderScale());
	}
	graphics::CommandBuffer::s_drawCalls.store(0);
	graphics::Mesh::s_trisDrawn.store(0);
}
} // namespace le
