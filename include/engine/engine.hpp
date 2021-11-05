#pragma once
#include <core/io.hpp>
#include <core/not_null.hpp>
#include <core/services.hpp>
#include <core/utils/profiler.hpp>
#include <core/version.hpp>
#include <engine/assets/asset_loaders_store.hpp>
#include <engine/editor/editor.hpp>
#include <engine/input/driver.hpp>
#include <engine/input/frame.hpp>
#include <engine/input/receiver.hpp>
#include <engine/scene/scene_space.hpp>
#include <engine/utils/engine_stats.hpp>
#include <engine/utils/error_handler.hpp>
#include <graphics/context/bootstrap.hpp>
#include <graphics/render/context.hpp>
#include <graphics/render/renderers.hpp>
#include <graphics/render/rgba.hpp>
#include <levk_imgui/levk_imgui.hpp>
#include <window/instance.hpp>

namespace le {
namespace graphics {
struct PhysicalDevice;
}

namespace gui {
class ViewStack;
}

class ListDrawer;
using graphics::Extent2D;

class Engine {
	template <typename T>
	struct tag_t {};

  public:
	using Window = window::Instance;
	using Boot = graphics::Bootstrap;
	using Context = graphics::RenderContext;
	using VRAM = graphics::VRAM;
	using RGBA = graphics::RGBA;
	using ClearDepth = graphics::ClearDepth;
	using ARenderer = graphics::ARenderer;
	using Stats = utils::EngineStats;
	using Profiler = std::conditional_t<levk_debug, utils::ProfileDB<>, utils::NullProfileDB>;

	struct GFX {
		Boot boot;
		Context context;
		DearImGui imgui;

		template <typename T, typename... Args>
		GFX(not_null<Window const*> winst, Boot::CreateInfo const& bci, tag_t<T>, Args&&... args)
			: boot(bci, makeSurface(*winst), winst->framebufferSize()),
			  context(&boot.swapchain, std::make_unique<T>(&boot.swapchain, std::forward<Args>(args)...)) {}

	  private:
		static Boot::MakeSurface makeSurface(Window const& winst);
	};

	struct CreateInfo;

	static Version version() noexcept;
	static Span<graphics::PhysicalDevice const> availableDevices();
	static auto profile(std::string_view name) { return Services::get<Profiler>()->profile(name); }

	Engine(CreateInfo const& info, io::Media const* custom = {});
	~Engine();

	bool bootReady() const noexcept { return m_win.has_value(); }

	input::Driver::Out poll(bool consume) noexcept;
	void pushReceiver(not_null<input::Receiver*> context);
	input::Receiver::Store& receiverStore() noexcept { return m_receivers; }
	void update(gui::ViewStack& out_stack);

	bool drawReady();
	bool nextFrame(graphics::RenderTarget* out = {});
	bool draw(ListDrawer& drawer, RGBA clear = colours::black, ClearDepth depth = {1.0f, 0});

	template <graphics::concrete_renderer Rd = graphics::Renderer_t<graphics::rtech::fwdSwpRp>, typename... Args>
	void boot(Boot::CreateInfo const& boot, Args&&... args);
	bool unboot() noexcept;
	bool booted() const noexcept { return m_gfx.has_value(); }

	Editor& editor() noexcept { return m_editor; }
	Editor const& editor() const noexcept { return m_editor; }
	GFX& gfx();
	GFX const& gfx() const;
	ARenderer& renderer() const;
	input::Frame const& inputFrame() const noexcept { return m_inputFrame; }
	Stats const& stats() const noexcept { return m_stats.stats; }
	AssetStore& store() noexcept { return m_store; }
	AssetStore const& store() const noexcept { return m_store; }

	Extent2D framebufferSize() const noexcept;
	Extent2D windowSize() const noexcept;
	Viewport const& gameView() const noexcept;
	glm::vec2 sceneSpace() const noexcept { return m_space(m_inputFrame.space); }

	window::Manager& windowManager() noexcept { return m_wm; }
	window::Manager const& windowManager() const noexcept { return m_wm; }
	Window& window();
	Window const& window() const;
	bool closing() const { return window().closing(); }

	SceneSpace m_space;
	io::Path m_configPath = "config.json";

  private:
	void updateStats();
	Boot::CreateInfo adjust(Boot::CreateInfo const& info);
	void bootImpl();
	void addDefaultAssets();
	std::optional<graphics::CommandBuffer> beginDraw(RGBA clear, ClearDepth depth);
	bool endDraw(graphics::CommandBuffer cb);
	void saveConfig() const;
	struct EngineConfig loadConfig() const;

	inline static ktl::fixed_vector<graphics::PhysicalDevice, 8> s_devices;

	io::Service m_io;
	window::Manager m_wm;
	std::optional<Window> m_win;
	std::optional<GFX> m_gfx;
	AssetStore m_store;
	input::Driver m_input;
	Editor m_editor;
	Stats::Counter m_stats;
	input::ReceiverStore m_receivers;
	input::Frame m_inputFrame;
	graphics::ScreenView m_view;
	std::optional<graphics::RenderTarget> m_drawing;
	Profiler m_profiler;
	utils::ErrorHandler m_errorHandler;
	time::Point m_lastPoll{};
};

struct Engine::CreateInfo {
	window::CreateInfo winInfo;
	std::optional<io::Path> logFile = "log.txt";
	LibLogger::Verbosity verbosity = LibLogger::libVerbosity;
};

// impl
template <graphics::concrete_renderer Rd, typename... Args>
void Engine::boot(Boot::CreateInfo const& info, Args&&... args) {
	unboot();
	ENSURE(m_win.has_value(), "No window");
	m_gfx.emplace(&*m_win, adjust(info), tag_t<Rd>{}, std::forward<Args>(args)...);
	bootImpl();
}

inline Engine::GFX& Engine::gfx() {
	ENSURE(m_gfx.has_value(), "Not booted");
	return *m_gfx;
}
inline Engine::GFX const& Engine::gfx() const {
	ENSURE(m_gfx.has_value(), "Not booted");
	return *m_gfx;
}
inline Engine::ARenderer& Engine::renderer() const {
	ENSURE(m_gfx.has_value(), "Not booted");
	return m_gfx->context.renderer();
}
inline Engine::Window& Engine::window() {
	ENSURE(m_win.has_value(), "Not booted");
	return *m_win;
}
inline Engine::Window const& Engine::window() const {
	ENSURE(m_win.has_value(), "Not booted");
	return *m_win;
}
} // namespace le
