#pragma once
#include <core/io.hpp>
#include <core/not_null.hpp>
#include <core/services.hpp>
#include <core/version.hpp>
#include <engine/editor/editor.hpp>
#include <engine/input/driver.hpp>
#include <engine/input/frame.hpp>
#include <engine/input/receiver.hpp>
#include <engine/scene/scene_space.hpp>
#include <engine/utils/engine_stats.hpp>
#include <graphics/context/bootstrap.hpp>
#include <graphics/render/context.hpp>
#include <graphics/render/renderers.hpp>
#include <graphics/render/rgba.hpp>
#include <levk_imgui/levk_imgui.hpp>
#include <window/instance.hpp>

namespace le {
namespace window {
class InstanceBase;
class DesktopInstance;
} // namespace window

namespace graphics {
struct PhysicalDevice;
}

namespace gui {
class ViewStack;
}

using graphics::Extent2D;

class Engine : public Service<Engine> {
	template <typename T>
	struct tag_t {};

  public:
	using Window = window::InstanceBase;
	using Desktop = window::DesktopInstance;
	using Boot = graphics::Bootstrap;
	using Context = graphics::RenderContext;
	using Drawer = graphics::FrameDrawer;
	using VRAM = graphics::VRAM;
	using RGBA = graphics::RGBA;
	using ARenderer = graphics::ARenderer;
	using Stats = utils::EngineStats;

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

	struct Options {
		std::optional<std::size_t> gpuOverride;
	};

	struct CreateInfo;

	inline static Options s_options;

	static Version version() noexcept;
	static Stats const& stats() noexcept { return s_stats; }
	static Span<graphics::PhysicalDevice const> availableDevices();

	Engine(not_null<Window*> winInst, CreateInfo const& info);

	input::Driver::Out poll(bool consume) noexcept;
	void pushReceiver(not_null<input::Receiver*> context);
	void update(gui::ViewStack& out_stack);

	bool editorActive() const noexcept { return m_editor.active(); }
	bool editorEngaged() const noexcept { return m_editor.active() && Editor::s_engaged; }

	bool drawReady();
	std::optional<Context::Frame> beginDraw();
	bool render(Context::Frame const& draw, Drawer& drawer, RGBA clear = colours::black, vk::ClearDepthStencilValue depth = {1.0f, 0});

	template <graphics::concrete_renderer Rd = graphics::Renderer_t<graphics::rtech::fwdSwpRp>, typename... Args>
	bool boot(Boot::CreateInfo boot, Args&&... args);
	bool unboot() noexcept;
	bool booted() const noexcept { return m_gfx.has_value(); }

	GFX& gfx();
	GFX const& gfx() const;
	ARenderer& renderer() const;
	input::Frame const& inputFrame() const noexcept { return m_inputFrame; }
	Desktop* desktop() const noexcept { return m_desktop; }

	Extent2D framebufferSize() const noexcept;
	Extent2D windowSize() const noexcept;
	Viewport const& gameView() const noexcept;
	glm::vec2 sceneSpace() const noexcept { return m_space(m_inputFrame.space); }

	SceneSpace m_space;
	not_null<Window*> m_win;
	Time_ms m_recreateInterval = 10ms;

  private:
	void updateStats();
	void bootImpl();

	inline static Stats s_stats = {};
	inline static kt::fixed_vector<graphics::PhysicalDevice, 8> s_devices;

	io::Service m_io;
	std::optional<GFX> m_gfx;
	Editor m_editor;
	input::Driver m_input;
	struct {
		struct {
			time::Point stamp{};
			Time_s elapsed{};
			u32 count;
		} frame;
	} m_stats;
	input::Receivers m_receivers;
	input::Frame m_inputFrame;
	graphics::ScreenView m_view;
	struct {
		glm::ivec2 size{};
		time::Point resized{};
	} m_fb;
	graphics::CommandBuffer m_drawing;
	Desktop* m_desktop{};
};

struct Engine::CreateInfo {
	std::optional<io::Path> logFile = "log.txt";
	LibLogger::Verbosity verbosity = LibLogger::libVerbosity;
};

// impl
template <graphics::concrete_renderer Rd, typename... Args>
bool Engine::boot(Boot::CreateInfo boot, Args&&... args) {
	if (!m_gfx) {
		if (s_options.gpuOverride) { boot.device.pickOverride = s_options.gpuOverride; }
		m_gfx.emplace(m_win.get(), boot, tag_t<Rd>{}, std::forward<Args>(args)...);
		bootImpl();
		return true;
	}
	return false;
}

inline Engine::GFX& Engine::gfx() {
	ensure(m_gfx.has_value(), "Not booted");
	return *m_gfx;
}
inline Engine::GFX const& Engine::gfx() const {
	ensure(m_gfx.has_value(), "Not booted");
	return *m_gfx;
}
inline Engine::ARenderer& Engine::renderer() const {
	ensure(m_gfx.has_value(), "Not booted");
	return m_gfx->context.renderer();
}
} // namespace le
