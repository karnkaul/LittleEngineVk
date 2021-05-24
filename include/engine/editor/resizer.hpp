#pragma once
#include <engine/input/state.hpp>
#include <engine/render/viewport.hpp>

namespace le {
namespace window {
class DesktopInstance;
}

namespace edi {
class Resizer {
  public:
	enum class Handle { eNone, eLeft, eRight, eBottom, eLeftBottom, eRightBottom };

	inline static f32 s_offsetX = 25.0f;
	inline static f32 s_minScale = 0.25f;

	bool operator()(window::DesktopInstance& out_w, Viewport& out_vp, input::State const& state);

  private:
	struct ViewData {
		glm::vec2 wSize;
		glm::vec2 fbSize;
		glm::vec2 ratio;
		glm::vec2 offset;

		ViewData(window::DesktopInstance const& win, Viewport const& view);
	};

	window::CursorType check(ViewData const& data, Viewport& out_vp, input::State const& state);
	void check(Viewport& out_vp, window::CursorType& out_c, bool active, Handle h, bool click);

	Viewport m_prev;
	Handle m_handle = {};
};
} // namespace edi
} // namespace le
