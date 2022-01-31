#include <dumb_tasks/executor.hpp>
#include <levk/core/build_version.hpp>
#include <levk/core/io.hpp>
#include <levk/core/io/zip_media.hpp>
#include <levk/core/log_channel.hpp>
#include <levk/core/services.hpp>
#include <levk/core/utils/data_store.hpp>
#include <levk/core/utils/error.hpp>
#include <levk/engine/assets/asset_loaders_store.hpp>
#include <levk/engine/builder.hpp>
#include <levk/engine/engine.hpp>
#include <levk/engine/gui/view.hpp>
#include <levk/engine/input/driver.hpp>
#include <levk/engine/input/receiver.hpp>
#include <levk/engine/render/frame.hpp>
#include <levk/engine/render/layer.hpp>
#include <levk/engine/utils/engine_config.hpp>
#include <levk/engine/utils/engine_stats.hpp>
#include <levk/engine/utils/error_handler.hpp>
#include <levk/graphics/utils/utils.hpp>
#include <levk/window/glue.hpp>
#include <levk/window/window.hpp>

namespace le {
namespace {
struct GFX {
	std::unique_ptr<graphics::Device> device;
	std::unique_ptr<graphics::VRAM> vram;
	graphics::RenderContext context;
};

ktl::fixed_vector<graphics::PhysicalDevice, 8> s_devices;

template <typename T>
void profilerNext(T& out_profiler, Time_s total) {
	if constexpr (!std::is_same_v<T, utils::NullProfileDB>) { out_profiler.next(total); }
}

std::optional<utils::EngineConfig> load(io::Path const& path) {
	if (path.empty()) { return std::nullopt; }
	if (auto json = dj::json(); json.load(path.generic_string())) { return io::fromJson<utils::EngineConfig>(json); }
	return std::nullopt;
}

bool save(utils::EngineConfig const& config, io::Path const& path) {
	if (path.empty()) { return false; }
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

graphics::Device::MakeSurface makeSurface(window::Window const& winst) {
	return [&winst](vk::Instance vkinst) { return window::makeSurface(vkinst, winst); };
}

graphics::RenderContext::GetSpirV getShader(AssetStore const& store) {
	return [&store](Hash uri) {
		ENSURE(store.exists<graphics::SpirV>(uri), "Shader doesn't exist");
		return *store.find<graphics::SpirV>(uri);
	};
}

std::optional<GFX> makeGFX(Engine::BootInfo const& info, window::Window const& window, AssetStore const& store) {
	auto device = graphics::Device::make(info.device, makeSurface(window));
	if (!device) {
		logW("[Engine] Failed to create graphics Device");
		return std::nullopt;
	}
	auto vram = graphics::VRAM::make(device.get(), info.vram);
	if (!vram) {
		logW("[Engine] Failed to create graphics VRAM");
		return std::nullopt;
	}
	auto vr = vram.get();
	graphics::RenderContext rc(vr, getShader(store), info.vsync, window.framebufferSize());
	return std::optional<GFX>(GFX{std::move(device), std::move(vram), std::move(rc)});
}

struct Delegates {
	ktl::delegate<> rendererChanged;
};
} // namespace

struct Engine::Impl {
	io::Service io;
	std::optional<window::Manager> wm;
	std::optional<Window> win;
	std::optional<GFX> gfx;
	AssetStore store;
	dts::thread_pool threadPool;
	Executor executor = Executor(&threadPool);
	Delegates delegates;
	input::Driver input;
	input::ReceiverStore receivers;
	input::Frame inputFrame;
	graphics::ScreenView view;
	Profiler profiler;
	time::Point lastPoll{};
	utils::EngineStats::Counter stats;
	utils::ErrorHandler errorHandler;
	Service service;
	io::Path configPath;

	Impl(std::optional<io::Path> logPath, LogChannel active) : io(logPath.value_or("levk-log.txt"), active), service(this) {}
};

BuildVersion Engine::buildVersion() noexcept { return g_buildVersion; }

Engine::Engine(std::unique_ptr<Impl>&& impl) noexcept : m_impl(std::move(impl)) {}

Engine::Engine(Engine&&) noexcept = default;
Engine& Engine::operator=(Engine&&) noexcept = default;

Engine::~Engine() noexcept {
	if (m_impl) {
		unboot();
		Services::untrack<Service>();
	}
}

bool Engine::boot(BootInfo info) {
	EXPECT(m_impl && m_impl->win);
	unboot();
	info.device.instance.extensions = window::instanceExtensions(*m_impl->win);
	if (auto gpuOverride = DataObject<CustomDevice>("gpuOverride")) { info.device.customDeviceName = gpuOverride->name; }
	m_impl->gfx = makeGFX(info, *m_impl->win, m_impl->store);
	if (!m_impl->gfx) { return false; }
	auto const& surface = m_impl->gfx->context.surface();
	logI("[Engine] Swapchain image count: [{}] VSync: [{}]", surface.imageCount(), graphics::vSyncNames[surface.format().vsync]);
	logD("[Engine] Device supports lazily allocated memory: {}", m_impl->gfx->device->physicalDevice().supportsLazyAllocation());
	Services::track<Context, VRAM, AssetStore, Profiler>(&m_impl->gfx->context, m_impl->gfx->vram.get(), &m_impl->store, &m_impl->profiler);
	addDefaultAssets();
	m_impl->win->show();
	m_impl->executor.start();
	return true;
}

bool Engine::unboot() noexcept {
	if (booted()) {
		saveConfig();
		m_impl->executor.stop();
		m_impl->store.clear();
		Services::untrack<Context, VRAM, AssetStore, Profiler>();
		m_impl->gfx->vram->shutdown();
		m_impl->gfx.reset();
		io::ZIPMedia::fsDeinit();
		return true;
	}
	return false;
}

bool Engine::booted() const noexcept { return m_impl->gfx.has_value(); }

Engine::Service Engine::service() const noexcept { return m_impl->service; }

void Engine::saveConfig() const {
	utils::EngineConfig config;
	config.win.position = m_impl->win->position();
	config.win.size = m_impl->win->windowSize();
	config.win.maximized = m_impl->win->maximized();
	if (save(config, m_impl->configPath)) { logI("[Engine] Config saved to {}", m_impl->configPath.generic_string()); }
}

void Engine::addDefaultAssets() {
	static_assert(detail::reloadable_asset_v<graphics::Texture>, "ODR violation! include asset_loaders.hpp");
	static_assert(!detail::reloadable_asset_v<int>, "ODR violation! include asset_loaders.hpp");
	auto device = m_impl->gfx->device.get();
	auto vram = m_impl->gfx->vram.get();
	auto sampler = m_impl->store.add("samplers/default", graphics::Sampler(device, {vk::Filter::eLinear, vk::Filter::eLinear}));
	{
		auto si = graphics::Sampler::info({vk::Filter::eLinear, vk::Filter::eLinear});
		si.maxLod = 0.0f;
		m_impl->store.add("samplers/no_mip_maps", graphics::Sampler(device, si));
	}
	{
		auto si = graphics::Sampler::info({vk::Filter::eLinear, vk::Filter::eLinear});
		si.mipmapMode = vk::SamplerMipmapMode::eLinear;
		si.addressModeU = si.addressModeV = si.addressModeW = vk::SamplerAddressMode::eClampToBorder;
		si.borderColor = vk::BorderColor::eIntOpaqueBlack;
		m_impl->store.add("samplers/font", graphics::Sampler(device, si));
	}
	/*Textures*/ {
		using Tex = graphics::Texture;
		auto v = vram;
		vk::Sampler s = sampler->sampler();
		m_impl->store.add("textures/red", Tex(v, s, colours::red, {1, 1}));
		m_impl->store.add("textures/black", Tex(v, s, colours::black, {1, 1}));
		m_impl->store.add("textures/white", Tex(v, s, colours::white, {1, 1}));
		m_impl->store.add("textures/magenta", Tex(v, s, colours::magenta, {1, 1}));
		m_impl->store.add("textures/blank", Tex(v, s, 0x0, {1, 1}));
		Tex blankCube(v, s);
		blankCube.construct(Tex::unitCubemap(0x0));
		m_impl->store.add("cubemaps/blank", std::move(blankCube));
	}
	/* meshes */ {
		auto cube = m_impl->store.add<graphics::MeshPrimitive>("meshes/cube", graphics::MeshPrimitive(vram));
		cube->construct(graphics::makeCube());
		auto cone = m_impl->store.add<graphics::MeshPrimitive>("meshes/cone", graphics::MeshPrimitive(vram));
		cone->construct(graphics::makeCone());
		auto wf_cube = m_impl->store.add<graphics::MeshPrimitive>("wireframes/cube", graphics::MeshPrimitive(vram));
		wf_cube->construct(graphics::makeCube(1.0f, {}, graphics::Topology::eLineList));
	}
	/* materials */ { m_impl->store.add("materials/default", Material{}); }
	/* render layers */ {
		RenderLayer layer;
		m_impl->store.add("render_layers/default", layer);
		layer.flags = RenderFlag::eAlphaBlend;
		layer.order = 100;
		m_impl->store.add("render_layers/ui", layer);
		layer.flags = {};
		layer.order = -100;
		m_impl->store.add("render_layers/skybox", layer);
	}
}

Span<graphics::PhysicalDevice const> Engine::Builder::availableDevices() {
	auto const channels = dlog::channels();
	if (s_devices.empty()) {
		dlog::set_channels({LC_EndUser});
		s_devices = graphics::Device::physicalDevices();
	}
	dlog::set_channels(channels);
	return s_devices;
}

std::optional<Engine> Engine::Builder::operator()() {
	auto impl = std::make_unique<Impl>(std::move(m_logFile), m_logChannels);
	auto wm = window::Manager::make();
	if (!wm) {
		logE("[Engine] Window manager not ready");
		return std::nullopt;
	}
	impl->wm = std::move(wm);
	if (m_media) { impl->store.resources().media(m_media); }
	logI("LittleEngineVk v{} | {}", buildVersion().toString(levk_debug), time::format(time::sysTime(), "{:%a %F %T %Z}"));
	logI(LC_EndUser, "Platform: {} {} ({})", levk_arch_name, levk_OS_name, os::cpuID());
	auto winInfo = m_windowInfo;
	winInfo.options.autoShow = false;
	if (auto config = load(m_configPath)) {
		logI(LC_LibUser, "[Engine] Config loaded from {}", m_configPath.generic_string());
		if (config->win.size.x > 0 && config->win.size.y > 0) { winInfo.config.size = config->win.size; }
		winInfo.config.position = config->win.position;
	}
	impl->win = impl->wm->makeWindow(winInfo);
	if (!impl->win) {
		logE(LC_EndUser, "[Engine] Failed to create window");
		return std::nullopt;
	}
	impl->errorHandler.deleteFile();
	impl->configPath = std::move(m_configPath);
	if (!impl->errorHandler.activeHandler()) { impl->errorHandler.setActive(); }
	std::vector<graphics::utils::STBImg> icons;
	icons.reserve(m_iconURIs.size());
	for (io::Path& uri : m_iconURIs) {
		if (auto bytes = impl->store.resources().load(std::move(uri), Resource::Type::eBinary)) { icons.push_back(graphics::utils::STBImg(bytes->bytes())); }
	}
	if (!icons.empty()) {
		std::vector<TBitmap<BmpView>> const views = {icons.begin(), icons.end()};
		impl->win->setIcon(views);
	}
	Services::track(&impl->service);
	return Engine(std::move(impl));
}

void Engine::Service::setRenderer(std::unique_ptr<Renderer>&& renderer) const {
	m_impl->gfx->context.setRenderer(std::move(renderer));
	m_impl->delegates.rendererChanged();
}

bool Engine::Service::booted() const noexcept { return m_impl->gfx.has_value(); }

void Engine::Service::poll(Viewport const& view, Opt<input::EventParser> custom) const {
	f32 const rscale = m_impl->gfx ? m_impl->gfx->context.renderer().renderScale() : 1.0f;
	input::Driver::In in{m_impl->win->pollEvents(), {framebufferSize(), sceneSpace()}, rscale, &*m_impl->win, custom};
	m_impl->inputFrame = m_impl->input.update(in, view);
	for (auto it = m_impl->receivers.rbegin(); it != m_impl->receivers.rend(); ++it) {
		if ((*it)->block(m_impl->inputFrame.state)) { break; }
	}
	if (m_impl->inputFrame.state.focus == input::Focus::eGained) { m_impl->store.update(); }
	profilerNext(m_impl->profiler, time::diffExchg(m_impl->lastPoll));
	m_impl->executor.rethrow();
}

void Engine::Service::pushReceiver(not_null<input::Receiver*> context) const { context->attach(m_impl->receivers); }
Engine::Profiler::Profiler Engine::Service::profile(std::string_view name) const { return m_impl->profiler.profile(name); }
window::Manager& Engine::Service::windowManager() const noexcept { return *m_impl->wm; }
Engine::Device& Engine::Service::device() const noexcept { return *m_impl->gfx->device; }
Engine::VRAM& Engine::Service::vram() const noexcept { return *m_impl->gfx->vram; }
Engine::Context& Engine::Service::context() const noexcept { return m_impl->gfx->context; }
Engine::Renderer& Engine::Service::renderer() const { return m_impl->gfx->context.renderer(); }
Engine::Window& Engine::Service::window() const {
	ENSURE(m_impl->win.has_value(), "Not booted");
	return *m_impl->win;
}
input::Frame const& Engine::Service::inputFrame() const noexcept { return m_impl->inputFrame; }
AssetStore& Engine::Service::store() const noexcept { return m_impl->store; }
input::Receiver::Store& Engine::Service::receiverStore() const noexcept { return m_impl->receivers; }
dts::executor& Engine::Service::executor() const noexcept { return m_impl->executor; }

bool Engine::Service::closing() const { return window().closing(); }
glm::vec2 Engine::Service::sceneSpace() const noexcept { return m_impl->inputFrame.space.display.swapchain; }
Extent2D Engine::Service::framebufferSize() const noexcept {
	if (m_impl->gfx) { return m_impl->gfx->context.surface().extent(); }
	return m_impl->win->framebufferSize();
}
Extent2D Engine::Service::windowSize() const noexcept { return m_impl->win->windowSize(); }
Engine::Stats const& Engine::Service::stats() const noexcept { return m_impl->stats.stats; }
Engine::Signal Engine::Service::onRendererChanged() const { return m_impl->delegates.rendererChanged.make_signal(); }

void Engine::Service::updateStats() const {
	m_impl->stats.update();
	m_impl->stats.stats.gfx.bytes.buffers = m_impl->gfx->vram->bytes(graphics::Memory::Type::eBuffer);
	m_impl->stats.stats.gfx.bytes.images = m_impl->gfx->vram->bytes(graphics::Memory::Type::eImage);
	m_impl->stats.stats.gfx.drawCalls = graphics::CommandBuffer::s_drawCalls.load();
	m_impl->stats.stats.gfx.triCount = graphics::MeshPrimitive::s_trisDrawn.load();
	m_impl->stats.stats.gfx.extents.window = m_impl->win->windowSize();
	if (m_impl->gfx) {
		m_impl->stats.stats.gfx.extents.swapchain = m_impl->gfx->context.surface().extent();
		m_impl->stats.stats.gfx.extents.renderer =
			Renderer::scaleExtent(m_impl->stats.stats.gfx.extents.swapchain, m_impl->gfx->context.renderer().renderScale());
	}
	graphics::CommandBuffer::s_drawCalls.store(0);
	graphics::MeshPrimitive::s_trisDrawn.store(0);
}

RenderFrame::RenderFrame(Engine::Service engine, graphics::RenderBegin const& rb) : m_engine(std::move(engine)) {
	m_engine.updateStats();
	m_renderPass = m_engine.m_impl->gfx->context.beginMainPass(rb, m_engine.m_impl->win->framebufferSize());
}

RenderFrame::~RenderFrame() {
	if (m_renderPass) { m_engine.m_impl->gfx->context.endMainPass(*m_renderPass, m_engine.m_impl->win->framebufferSize()); }
}
} // namespace le
