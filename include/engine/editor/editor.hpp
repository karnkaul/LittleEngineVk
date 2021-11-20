#pragma once
#include <core/time.hpp>
#include <dens/entity.hpp>
#include <engine/editor/palette_tab.hpp>
#include <engine/editor/types.hpp>
#include <engine/input/frame.hpp>
#include <engine/render/viewport.hpp>
#include <ktl/either.hpp>
#include <levk_imgui/levk_imgui.hpp>

#if defined(LEVK_EDITOR)
#include <engine/editor/log_stats.hpp>
#include <engine/editor/main_menu.hpp>
#include <engine/editor/resizer.hpp>
#endif

namespace le {
namespace window {
class Instance;
}
namespace graphics {
struct ScreenView;
}
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

	edi::MenuList m_menu;

  private:
	graphics::ScreenView update(edi::SceneRef scene, Engine const& engine);

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

	friend class Engine;
};
} // namespace le
