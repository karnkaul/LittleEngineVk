#pragma once
#include <dens/entity.hpp>
#include <ktl/either.hpp>
#include <levk/core/time.hpp>
#include <levk/engine/editor/palette_tab.hpp>
#include <levk/engine/editor/types.hpp>
#include <levk/engine/engine.hpp>
#include <levk/engine/input/frame.hpp>
#include <levk/engine/render/viewport.hpp>
#include <memory>

#if defined(LEVK_EDITOR)
#include <ktl/delegate.hpp>
#include <levk/engine/editor/log_stats.hpp>
#include <levk/engine/editor/main_menu.hpp>
#include <levk/engine/editor/resizer.hpp>
#endif

namespace le {
namespace window {
class Window;
}
namespace graphics {
struct ScreenView;
class RenderContext;
class CommandBuffer;
} // namespace graphics
namespace gui {
class TreeRoot;
}
namespace editor {
class Inspector;
} // namespace editor
class Engine;

class Editor {
  public:
	inline static Viewport s_comboView = {{0.2f, 0.0f}, {0.0f, 20.0f}, 0.6f};

	Editor();
	Editor(Engine::Service service);
	~Editor() noexcept;

	bool engaged() const noexcept { return m_storage.engaged; }
	void engage(bool set) noexcept { m_storage.engaged = set; }
	void toggle() noexcept { engage(!engaged()); }

	Viewport const& view() const noexcept;
	bool active() const noexcept;
	bool showImGuiDemo() const noexcept;

	void init(graphics::RenderContext& context, window::Window const& window);
	graphics::ScreenView update(editor::SceneRef scene);
	void render(graphics::CommandBuffer const& cb);
	void deinit() noexcept;

	editor::MenuList m_menu;

  private:
	struct {
#if defined(LEVK_EDITOR)
		editor::Resizer resizer;
		editor::LogStats logStats;
		editor::MainMenu menu;
		Engine::Signal rendererChanged;
#endif
		Viewport gameView = s_comboView;
		bool engaged{};
		bool renderReady{};
	} m_storage;

	struct {
		editor::Inspecting inspect;
		dens::registry const* prev{};
	} m_cache;

	struct Rail {
		std::string_view id;
		std::unique_ptr<editor::PaletteTab> tab;
	};

	Rail m_left = {"Left", {}};
	Rail m_right = {"Right", {}};

	std::unique_ptr<class DearImGui> m_imgui;
};
} // namespace le
