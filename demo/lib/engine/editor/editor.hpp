#pragma once
#include <core/ref.hpp>
#include <core/time.hpp>
#include <dumb_ecf/registry.hpp>
#include <engine/editor/log_stats.hpp>
#include <engine/editor/resizer.hpp>
#include <engine/input/input.hpp>
#include <engine/render/viewport.hpp>
#include <levk_imgui/levk_imgui.hpp>

namespace le {
namespace window {
class DesktopInstance;
}
namespace graphics {
struct Bootstrap;
}
class DearImGui;

class Editor {
  public:
	using DesktopInstance = window::DesktopInstance;
	using Bootstrap = graphics::Bootstrap;

	inline static Viewport s_comboView = {{0.2f, 0.0f}, {0.0f, 25.0f}, 0.6f};

	Viewport const& view() const noexcept;
	bool active() const noexcept;

	void update(DesktopInstance& win, Input::State const& state);

	bool m_engaged = false;

  private:
	struct {
		edi::Resizer resizer;
		edi::LogStats logStats;
		Viewport gameView = s_comboView;
	} m_storage;
};
} // namespace le
