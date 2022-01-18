#pragma once
#include <levk/core/services.hpp>
#include <levk/core/utils/profiler.hpp>
#include <levk/core/version.hpp>
#include <levk/engine/input/driver.hpp>
#include <levk/engine/input/receiver.hpp>
#include <levk/engine/scene/space.hpp>

namespace le {
namespace io {
class Media;
}

namespace graphics {
class Device;
class VRAM;
class RenderContext;
class Renderer;
} // namespace graphics

namespace window {
class Manager;
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

class SceneManager;

using Extent2D = glm::uvec2;

class Engine {
  public:
	using Window = window::Window;
	using Device = graphics::Device;
	using VRAM = graphics::VRAM;
	using Context = graphics::RenderContext;
	using Renderer = graphics::Renderer;
	using Stats = utils::EngineStats;
	using Profiler = std::conditional_t<levk_debug, utils::ProfileDB<>, utils::NullProfileDB>;

	struct CustomDevice;
	struct CreateInfo;
	struct BootInfo;
	class Builder;
	class Service;

	static BuildVersion buildVersion() noexcept;
	static auto profile(std::string_view name) { return Services::get<Profiler>()->profile(name); }

	Engine(Engine&&) noexcept;
	Engine& operator=(Engine&&) noexcept;
	~Engine() noexcept;

	bool bootReady() const noexcept;

	void poll(Opt<input::EventParser> custom = {});

	bool boot(BootInfo info);
	bool unboot() noexcept;
	bool booted() const noexcept;

	Service service() const noexcept;

  private:
	void addDefaultAssets();
	void saveConfig() const;

	struct Impl;
	Engine(std::unique_ptr<Impl>&& impl) noexcept;

	std::unique_ptr<Impl> m_impl;
};

class Engine::Service {
  public:
	void pushReceiver(not_null<input::Receiver*> context) const;
	void updateViewStack(gui::ViewStack& out_stack) const;
	void setRenderer(std::unique_ptr<Renderer>&& renderer) const;

	window::Manager& windowManager() const noexcept;
	Editor& editor() const noexcept;
	Device& device() const noexcept;
	VRAM& vram() const noexcept;
	Context& context() const noexcept;
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
	friend class RenderFrame;
};
} // namespace le
