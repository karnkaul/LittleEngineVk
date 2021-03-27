#include <core/array_map.hpp>
#include <core/maths.hpp>
#include <engine/editor/resizer.hpp>
#include <levk_imgui/levk_imgui.hpp>
#include <window/desktop_instance.hpp>

namespace le::edi {
using Key = window::Key;
using CursorType = window::CursorType;

namespace {
using Hd = Resizer::Handle;
constexpr ArrayMap<6, Resizer::Handle, CursorType> handleCursor = {{Hd::eNone, CursorType::eDefault},		   {Hd::eLeft, CursorType::eResizeEW},
																   {Hd::eRight, CursorType::eResizeEW},		   {Hd::eBottom, CursorType::eResizeNS},
																   {Hd::eLeftBottom, CursorType::eResizeNESW}, {Hd::eRightBottom, CursorType::eResizeNWSE}};

[[nodiscard]] Resizer::Handle endResize() {
	IMGUI(ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange);
	return Resizer::Handle::eNone;
}

bool inZone(f32 s0, f32 t0, f32 s1, f32 t1) {
	static constexpr f32 nDelta = 10.0f;
	if (maths::equals(s0, t0, nDelta) && maths::equals(s1, t1, nDelta)) {
		IMGUI(ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange);
		return true;
	}
	return false;
}

f32 clampScale(f32 scale, glm::vec2 fb, glm::vec2 offset) noexcept {
	scale = std::clamp(scale, Resizer::s_minScale, 1.0f);
	glm::vec2 const maxSize = fb - glm::vec2(offset);
	glm::vec2 const size = scale * fb;
	if (size.x > maxSize.x || size.y > maxSize.y) {
		return std::min(maxSize.x / fb.x, maxSize.y / fb.y);
	}
	return scale;
}

f32 clampLeft(f32 left, f32 scale, f32 fbx, f32 offset) noexcept {
	f32 const realLeft = left * fbx;
	f32 const realLimit = (1.0f - scale) * fbx;
	offset = std::min(offset, realLimit * 0.49f);
	f32 const clamped = std::clamp(realLeft, offset, realLimit - offset);
	return clamped / fbx;
}
} // namespace

Resizer::ViewData::ViewData(window::DesktopInstance const& win, Viewport const& view) {
	wSize = {f32(win.windowSize().x), f32(win.windowSize().y)};
	fbSize = {f32(win.framebufferSize().x), f32(win.framebufferSize().y)};
	ratio = {wSize.x / fbSize.x, wSize.y / fbSize.y};
	offset = view.topLeft.offset * ratio;
}

bool Resizer::operator()(window::DesktopInstance& out_w, Viewport& out_vp, Input::State const& state) {
	ViewData const data(out_w, out_vp);
	glm::vec2 const nCursor = {state.cursor.position.x / data.wSize.x, state.cursor.position.y / data.wSize.y};
	CursorType toSet = CursorType::eDefault;
	if (m_handle != Handle::eNone && !state.held(Key::eMouseButton1)) {
		m_handle = endResize();
	}
	if (state.pressed(Key::eEscape)) {
		out_vp = m_prev;
		m_handle = endResize();
	} else {
		f32 const xOffset = out_vp.topLeft.offset.x * data.fbSize.x + s_offsetX;
		switch (m_handle) {
		case Handle::eNone: {
			toSet = check(data, out_vp, state);
			break;
		}
		case Handle::eLeft: {
			out_vp.topLeft.n.x = clampLeft(nCursor.x, out_vp.scale, data.fbSize.x, xOffset);
			toSet = mapped<CursorType>(handleCursor, m_handle);
			break;
		}
		case Handle::eRight: {
			out_vp.topLeft.n.x = clampLeft(nCursor.x - out_vp.scale, out_vp.scale, data.fbSize.x, xOffset);
			toSet = mapped<CursorType>(handleCursor, m_handle);
			break;
		}
		case Handle::eBottom: {
			auto const centre = out_vp.topLeft.n.x + out_vp.scale * 0.5f;
			out_vp.scale = clampScale(nCursor.y, data.fbSize, 2.0f * out_vp.topLeft.offset);
			out_vp.topLeft.n.x = clampLeft(centre - out_vp.scale * 0.5f, out_vp.scale, data.fbSize.x, xOffset);
			toSet = mapped<CursorType>(handleCursor, m_handle);
			break;
		}
		case Handle::eLeftBottom: {
			out_vp.scale = clampScale(nCursor.y, data.fbSize, 2.0f * out_vp.topLeft.offset);
			out_vp.topLeft.n.x = clampLeft(nCursor.x, out_vp.scale, data.fbSize.x, xOffset);
			toSet = mapped<CursorType>(handleCursor, m_handle);
			break;
		}
		case Handle::eRightBottom: {
			out_vp.scale = clampScale(nCursor.y, data.fbSize, 2.0f * out_vp.topLeft.offset);
			out_vp.topLeft.n.x = clampLeft(nCursor.x - out_vp.scale, out_vp.scale, data.fbSize.x, xOffset);
			toSet = mapped<CursorType>(handleCursor, m_handle);
			break;
		}
		default:
			break;
		}
	}
	out_w.cursorType(toSet);
	return m_handle > Handle::eNone;
}

CursorType Resizer::check(ViewData const& data, Viewport& out_vp, Input::State const& state) {
	auto const rect = out_vp.rect();
	auto const cursor = state.cursor.position;
	bool const click = state.pressed(Key::eMouseButton1).has_result();
	CursorType ret = CursorType::eDefault;
	check(out_vp, ret, cursor.x, rect.lt.x * data.wSize.x + data.offset.x, 0.0f, 0.0f, Handle::eLeft, click);
	check(out_vp, ret, cursor.x, rect.rb.x * data.wSize.x + data.offset.x, 0.0f, 0.0f, Handle::eRight, click);
	check(out_vp, ret, cursor.y, rect.rb.y * data.wSize.y + data.offset.y, 0.0f, 0.0f, Handle::eBottom, click);
	check(out_vp, ret, cursor.x, rect.lt.x * data.wSize.x + data.offset.x, cursor.y, rect.rb.y * data.wSize.y + data.offset.y, Handle::eLeftBottom, click);
	check(out_vp, ret, cursor.x, rect.rb.x * data.wSize.x + data.offset.x, cursor.y, rect.rb.y * data.wSize.y + data.offset.y, Handle::eRightBottom, click);
	return ret;
}

void Resizer::check(Viewport& out_vp, CursorType& out_c, f32 s0, f32 t0, f32 s1, f32 t1, Handle h, bool click) {
	if (inZone(s0, t0, s1, t1)) {
		out_c = mapped(handleCursor, h, CursorType::eDefault);
		if (click) {
			m_prev = out_vp;
			m_handle = h;
		}
	}
}
} // namespace le::edi
