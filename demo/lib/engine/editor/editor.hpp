#pragma once
#include <core/ref.hpp>
#include <core/time.hpp>
#include <dumb_ecf/registry.hpp>
#include <engine/editor/log_stats.hpp>
#include <engine/editor/main_menu.hpp>
#include <engine/editor/resizer.hpp>
#include <engine/editor/types.hpp>
#include <engine/input/input.hpp>
#include <engine/render/viewport.hpp>
#include <levk_imgui/levk_imgui.hpp>

namespace le {
namespace window {
class DesktopInstance;
}

class Editor {
  public:
	using DesktopInstance = window::DesktopInstance;

	inline static Viewport s_comboView = {{0.2f, 0.0f}, {0.0f, 20.0f}, 0.6f};
	inline static bool s_engaged = false;
	inline static std::vector<edi::MenuTree> s_menus;

	Viewport const& view() const noexcept;
	bool active() const noexcept;

	void update(DesktopInstance& win, Input::State const& state);

  private:
	struct {
		edi::Resizer resizer;
		edi::LogStats logStats;
		edi::MainMenu menu;
		Viewport gameView = s_comboView;
	} m_storage;
};
} // namespace le
