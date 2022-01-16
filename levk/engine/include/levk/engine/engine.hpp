#pragma once
#include <levk/core/services.hpp>
#include <levk/core/utils/profiler.hpp>
#include <levk/core/version.hpp>
#include <levk/engine/input/driver.hpp>
#include <levk/engine/input/receiver.hpp>
#include <levk/engine/scene/space.hpp>
#include <levk/graphics/context/bootstrap.hpp>
#include <levk/graphics/render/context.hpp>
#include <levk/window/window.hpp>

namespace le {
namespace io {
class Media;
}

namespace gui {
class ViewStack;
}

class AssetStore;
class Editor;
class SceneRegistry;

namespace utils {
struct EngineStats;
}

using Extent2D = graphics::Extent2D;

class Engine {
  public:
	using Window = window::Window;
	using Boot = graphics::Bootstrap;
	using VSync = graphics::VSync;
	using VRAM = graphics::VRAM;
	using RGBA = graphics::RGBA;
	using Context = graphics::RenderContext;
	using Renderer = graphics::Renderer;
	using ClearDepth = graphics::ClearDepth;
	using RenderPass = graphics::RenderPass;
	using Stats = utils::EngineStats;
	using Profiler = std::conditional_t<levk_debug, utils::ProfileDB<>, utils::NullProfileDB>;

	struct GFX;
	struct CustomDevice;
	struct CreateInfo;
	class Builder;
	class Service;

	static Version version() noexcept;
	static Span<graphics::PhysicalDevice const> availableDevices();
	static bool drawImgui(graphics::CommandBuffer cb);
	static auto profile(std::string_view name) { return Services::get<Profiler>()->profile(name); }

	Engine(Engine&&) noexcept;
	Engine& operator=(Engine&&) noexcept;
	~Engine() noexcept;

	bool bootReady() const noexcept;

	void poll(Opt<input::EventParser> custom = {});

	void boot(Boot::CreateInfo info, std::optional<VSync> vsync = std::nullopt);
	bool unboot() noexcept;
	bool booted() const noexcept;

	Service service() const noexcept;

  private:
	void addDefaultAssets();
	void saveConfig() const;

	inline static ktl::fixed_vector<graphics::PhysicalDevice, 8> s_devices;

	struct Impl;
	Engine(std::unique_ptr<Impl>&& impl) noexcept;

	std::unique_ptr<Impl> m_impl;
};

struct Engine::CustomDevice {
	std::string name;
};

struct Engine::CreateInfo {
	window::CreateInfo winInfo;
	std::optional<io::Path> logFile = "log.txt";
	LogChannel logChannels = log_channels_v;
};

class Engine::Builder {
  public:
	Builder& window(window::CreateInfo info) noexcept { return (m_windowInfo = std::move(info), *this); }
	Builder& logFile(io::Path path) noexcept { return (m_logFile = std::move(path), *this); }
	Builder& noLogFile() noexcept { return (m_logFile.reset(), *this); }
	Builder& logChannels(LogChannel const lc) noexcept { return (m_logChannels = lc, *this); }
	Builder& media(not_null<io::Media const*> m) noexcept { return (m_media = m, *this); }
	Builder& configFile(io::Path path) noexcept { return (m_configPath = std::move(path), *this); }

	std::optional<Engine> operator()() const;

  private:
	window::CreateInfo m_windowInfo;
	std::optional<io::Path> m_logFile = "log.txt";
	io::Path m_configPath = "levk_config.json";
	LogChannel m_logChannels = log_channels_v;
	io::Media const* m_media{};
};

struct Engine::GFX {
	Boot boot;
	Context context;

	GFX(not_null<Window const*> winst, Boot::CreateInfo const& bci, AssetStore const& store, std::optional<VSync> vsync);
};

class Engine::Service {
  public:
	void pushReceiver(not_null<input::Receiver*> context) const;
	void updateViewStack(gui::ViewStack& out_stack) const;
	void setRenderer(std::unique_ptr<Renderer>&& renderer) const;

	void nextFrame() const;
	std::optional<RenderPass> beginRenderPass(RGBA clear, ClearDepth depth = {1.0f, 0}) const { return beginRenderPass({}, clear, depth); }
	std::optional<RenderPass> beginRenderPass(Opt<SceneRegistry> scene, RGBA clear, ClearDepth depth = {1.0f, 0}) const;
	bool endRenderPass(RenderPass& out_rp) const;

	window::Manager& windowManager() const noexcept;
	Editor& editor() const noexcept;
	GFX& gfx() const;
	Renderer& renderer() const;
	input::Frame const& inputFrame() const noexcept;
	AssetStore& store() const noexcept;
	input::Receiver::Store& receiverStore() noexcept;

	bool closing() const;
	Extent2D framebufferSize() const noexcept;
	Extent2D windowSize() const noexcept;
	glm::vec2 sceneSpace() const noexcept;
	Window& window() const;

	Stats const& stats() const noexcept;

  private:
	Service(not_null<Impl*> impl) noexcept : m_impl(impl) {}
	void updateStats() const;

	not_null<Impl*> m_impl;
	friend class Engine;
};
} // namespace le
