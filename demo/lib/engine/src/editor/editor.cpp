#include <core/maths.hpp>
#include <engine/editor/editor.hpp>
#include <graphics/context/bootstrap.hpp>
#include <window/desktop_instance.hpp>

namespace le {
bool Editor::active() const noexcept {
	if constexpr (levk_imgui) {
		return DearImGui::inst() != nullptr;
	}
	return false;
}

Viewport const& Editor::view() const noexcept {
	static constexpr Viewport s_default;
	return active() && m_engaged ? m_storage.gameView : s_default;
}

void Editor::update(DesktopInstance& win, Input::State const& state) {
	if (active() && m_engaged) {
		m_storage.resizer(win, m_storage.gameView, state);
	}
}
} // namespace le
