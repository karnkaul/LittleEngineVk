#pragma once
#include <engine/input/input.hpp>
#include <engine/render/viewport.hpp>

namespace le {
namespace window {
class DesktopInstance;
}

class ViewportResizer {
  public:
	enum class Handle { eNone, eLeft, eRight, eBottom, eLeftBottom, eRightBottom };

	inline static f32 s_offsetX = 25.0f;
	inline static f32 s_minScale = 0.25f;

	bool operator()(window::DesktopInstance& out_w, Viewport& out_vp, Input::State const& state);

  private:
	struct ViewData {
		glm::vec2 wSize;
		glm::vec2 fbSize;
		glm::vec2 ratio;
		glm::vec2 offset;

		ViewData(window::DesktopInstance const& win, Viewport const& view);
	};

	window::CursorType check(ViewData const& data, Viewport& out_vp, Input::State const& state);
	void check(Viewport& out_vp, window::CursorType& out_c, f32 s0, f32 t0, f32 s1, f32 t1, Handle h, bool click);

	Viewport m_prev;
	Handle m_handle = {};
};
} // namespace le
