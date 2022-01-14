#pragma once
#include <core/services.hpp>
#include <core/utils/profiler.hpp>
#include <core/version.hpp>
#include <levk/engine/input/driver.hpp>
#include <levk/engine/input/receiver.hpp>
#include <levk/engine/scene/space.hpp>
#include <levk/graphics/context/bootstrap.hpp>
#include <levk/graphics/render/context.hpp>
#include <levk/window/instance.hpp>

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
	using Window = window::Instance;
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

	static Version version() noexcept;
	static Span<graphics::PhysicalDevice const> availableDevices();
	static bool drawImgui(graphics::CommandBuffer cb);
	static auto profile(std::string_view name) { return Services::get<Profiler>()->profile(name); }

	Engine(CreateInfo const& info, io::Media const* custom = {});
	~Engine();

	bool bootReady() const noexcept;

	input::Driver::Out poll(bool consume) noexcept;
	void pushReceiver(not_null<input::Receiver*> context);
	input::Receiver::Store& receiverStore() noexcept;
	void update(gui::ViewStack& out_stack);

	void boot(Boot::CreateInfo info, std::optional<VSync> vsync = std::nullopt);
	bool unboot() noexcept;
	bool booted() const noexcept;
	bool setRenderer(std::unique_ptr<Renderer>&& renderer);

	bool nextFrame();
	std::optional<RenderPass> beginRenderPass(RGBA clear, ClearDepth depth = {1.0f, 0}) { return beginRenderPass({}, clear, depth); }
	std::optional<RenderPass> beginRenderPass(Opt<SceneRegistry> scene, RGBA clear, ClearDepth depth = {1.0f, 0});
	bool endRenderPass(RenderPass& out_rp);

	Editor& editor() const noexcept;
	GFX& gfx() const;
	Renderer& renderer() const;
	input::Frame const& inputFrame() const noexcept;
	Stats const& stats() const noexcept;
	AssetStore& store() const noexcept;

	Extent2D framebufferSize() const noexcept;
	Extent2D windowSize() const noexcept;
	glm::vec2 sceneSpace() const noexcept;

	window::Manager& windowManager() const noexcept;
	Window& window() const;
	bool closing() const;

	scene::Space m_space;
	io::Path m_configPath = "levk_config.json";

  private:
	void updateStats();
	void addDefaultAssets();
	void saveConfig() const;

	inline static ktl::fixed_vector<graphics::PhysicalDevice, 8> s_devices;

	struct Impl;

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

struct Engine::GFX {
	Boot boot;
	Context context;

	GFX(not_null<Window const*> winst, Boot::CreateInfo const& bci, AssetStore const& store, std::optional<VSync> vsync);
};
} // namespace le
