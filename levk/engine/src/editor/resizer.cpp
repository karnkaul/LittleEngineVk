#include <core/array_map.hpp>
#include <core/maths.hpp>
#include <levk/engine/editor/resizer.hpp>
#include <levk/window/instance.hpp>

#if defined(LEVK_USE_IMGUI)
#include <imgui.h>
#endif

namespace le::edi {
using Key = window::Key;
using CursorType = window::CursorType;

namespace {
using Hd = Resizer::Handle;
constexpr EnumArray<Resizer::Handle, CursorType, 6> handleCursor = {CursorType::eDefault,  CursorType::eResizeEW,	CursorType::eResizeEW,
																	CursorType::eResizeNS, CursorType::eResizeNESW, CursorType::eResizeNWSE};

[[nodiscard]] Resizer::Handle endResize() {
#if defined(LEVK_USE_IMGUI)
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
#endif
	return Resizer::Handle::eNone;
}

bool inZone(f32 a, f32 b) {
	static constexpr f32 nDelta = 10.0f;
	if (maths::equals(a, b, nDelta)) { return true; }
	return false;
}

f32 clampScale(f32 scale, glm::vec2 fb, glm::vec2 offset) noexcept {
	scale = std::clamp(scale, Resizer::s_minScale, 1.0f);
	glm::vec2 const maxSize = fb - glm::vec2(offset);
	glm::vec2 const size = scale * fb;
	if (size.x > maxSize.x || size.y > maxSize.y) { return std::min(maxSize.x / fb.x, maxSize.y / fb.y); }
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

bool Resizer::operator()([[maybe_unused]] window::Instance& out_w, Viewport& out_vp, input::Frame const& frame) {
	auto const& size = frame.space.display.window;
	glm::vec2 const nCursor = {frame.state.cursor.screenPos.x / size.x, frame.state.cursor.screenPos.y / size.y};
	CursorType toSet = CursorType::eDefault;
	if (m_handle == Handle::eNone) { m_prev = out_vp; }
	if (m_handle != Handle::eNone && !frame.state.held(Key::eMouseButton1)) { m_handle = endResize(); }
	if (frame.state.pressed(Key::eEscape)) {
		out_vp = m_prev;
		m_handle = endResize();
	} else {
		f32 const xOffset = out_vp.topLeft.offset.x * size.x + s_offsetX;
		switch (m_handle) {
		case Handle::eNone: {
			toSet = check(out_vp, frame);
			break;
		}
		case Handle::eLeft: {
			out_vp.topLeft.norm.x = clampLeft(nCursor.x, out_vp.scale, size.x, xOffset);
			toSet = handleCursor[m_handle];
			break;
		}
		case Handle::eRight: {
			out_vp.topLeft.norm.x = clampLeft(nCursor.x - out_vp.scale, out_vp.scale, size.x, xOffset);
			toSet = handleCursor[m_handle];
			break;
		}
		case Handle::eBottom: {
			auto const centre = out_vp.topLeft.norm.x + out_vp.scale * 0.5f;
			out_vp.scale = clampScale(nCursor.y, size, 2.0f * out_vp.topLeft.offset);
			out_vp.topLeft.norm.x = clampLeft(centre - out_vp.scale * 0.5f, out_vp.scale, size.x, xOffset);
			toSet = handleCursor[m_handle];
			break;
		}
		case Handle::eLeftBottom: {
			out_vp.scale = clampScale(nCursor.y, size, 2.0f * out_vp.topLeft.offset);
			out_vp.topLeft.norm.x = clampLeft(nCursor.x, out_vp.scale, size.x, xOffset);
			toSet = handleCursor[m_handle];
			break;
		}
		case Handle::eRightBottom: {
			out_vp.scale = clampScale(nCursor.y, size, 2.0f * out_vp.topLeft.offset);
			out_vp.topLeft.norm.x = clampLeft(nCursor.x - out_vp.scale, out_vp.scale, size.x, xOffset);
			toSet = handleCursor[m_handle];
			break;
		}
		default: break;
		}
	}
	out_w.cursorType(toSet);
	return m_handle > Handle::eNone;
}

CursorType Resizer::check(Viewport const& vp, input::Frame const& frame) {
	auto const& size = frame.space.display.window;
	auto const cursor = frame.state.cursor.screenPos;
	bool const click = frame.state.pressed(Key::eMouseButton1);
	CursorType ret = CursorType::eDefault;
	auto const left = frame.space.viewport.offset.x;
	auto const right = left + frame.space.viewport.scale * size.x;
	auto const bottom = frame.space.viewport.offset.y + frame.space.viewport.scale * size.y;
	check(vp, ret, inZone(cursor.x, left) && cursor.y < bottom, Handle::eLeft, click);
	check(vp, ret, inZone(cursor.x, right) && cursor.y < bottom, Handle::eRight, click);
	check(vp, ret, inZone(cursor.y, bottom), Handle::eBottom, click);
	check(vp, ret, inZone(cursor.x, left) && inZone(cursor.y, bottom), Handle::eLeftBottom, click);
	check(vp, ret, inZone(cursor.x, right) && inZone(cursor.y, bottom), Handle::eRightBottom, click);
	return ret;
}

void Resizer::check(Viewport const& vp, CursorType& out_c, bool active, Handle h, bool click) {
	if (active) {
#if defined(LEVK_USE_IMGUI)
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
#endif
		out_c = handleCursor[h];
		if (click) {
			m_prev = vp;
			m_handle = h;
		}
	}
}
} // namespace le::edi
