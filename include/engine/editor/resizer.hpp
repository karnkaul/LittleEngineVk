#pragma once
#include <engine/input/frame.hpp>
#include <engine/render/viewport.hpp>

namespace le {
namespace edi {
class Resizer {
  public:
	inline static bool s_block = false;

	enum class Handle { eNone, eLeft, eRight, eBottom, eLeftBottom, eRightBottom };

	inline static f32 s_offsetX = 25.0f;
	inline static f32 s_minScale = 0.25f;

	bool operator()(window::Instance& out_w, Viewport& out_vp, input::Frame const& frame);

  private:
	window::CursorType check(Viewport const& out_vp, input::Frame const& frame);
	void check(Viewport const& vp, window::CursorType& out_c, bool active, Handle h, bool click);

	Viewport m_prev;
	Handle m_handle = {};
};
} // namespace edi
} // namespace le
