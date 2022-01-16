#pragma once
#include <dens/entity.hpp>
#include <ktl/either.hpp>
#include <levk/core/time.hpp>
#include <levk/engine/editor/palette_tab.hpp>
#include <levk/engine/editor/types.hpp>
#include <levk/engine/input/frame.hpp>
#include <levk/engine/render/viewport.hpp>
#include <memory>

#if defined(LEVK_EDITOR)
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
namespace edi {
class Inspector;
} // namespace edi
class Engine;

class Editor {
  public:
	inline static Viewport s_comboView = {{0.2f, 0.0f}, {0.0f, 20.0f}, 0.6f};

	Editor();
	~Editor() noexcept;

	bool engaged() const noexcept { return m_storage.engaged; }
	void engage(bool set) noexcept { m_storage.engaged = set; }
	void toggle() noexcept { engage(!engaged()); }

	Viewport const& view() const noexcept;
	bool active() const noexcept;
	bool showImGuiDemo() const noexcept;

	edi::MenuList m_menu;

  private:
	void init(graphics::RenderContext* context, window::Window* window);
	void deinit() noexcept;
	bool beginFrame();
	graphics::ScreenView update(edi::SceneRef scene);
	void render(graphics::CommandBuffer const& cb);

	struct {
#if defined(LEVK_EDITOR)
		edi::Resizer resizer;
		edi::LogStats logStats;
		edi::MainMenu menu;
#endif
		Viewport gameView = s_comboView;
		bool engaged{};
	} m_storage;

	struct {
		edi::Inspecting inspect;
		dens::registry const* prev{};
	} m_cache;

	struct Rail {
		std::string_view id;
		std::unique_ptr<edi::PaletteTab> tab;
	};

	Rail m_left = {"Left", {}};
	Rail m_right = {"Right", {}};

	std::unique_ptr<class DearImGui> m_imgui;

	friend class Engine;
};
} // namespace le
