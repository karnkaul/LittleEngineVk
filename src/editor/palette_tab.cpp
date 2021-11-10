#include <engine/editor/palette_tab.hpp>
#include <levk_imgui/levk_imgui.hpp>
#include <algorithm>

namespace le::edi {
bool PaletteTab::detach(std::string_view id) {
	if (auto it = std::find_if(m_items.begin(), m_items.end(), [&id](Entry const& e) { return e.id == id; }); it != m_items.end()) {
		m_items.erase(it);
		return true;
	}
	return false;
}

#define MU [[maybe_unused]]
using namespace dens;

bool PaletteTab::update(MU std::string_view id, MU glm::vec2 size, MU glm::vec2 pos, MU SceneRef scene) {
	bool ret = false;
#if defined(LEVK_USE_IMGUI)
	if (size.x < s_minSize.x || size.y < s_minSize.y) { return false; }
	static constexpr f32 s_xPad = 2.0f;
	static constexpr f32 const s_dy = 2.0f;
	static constexpr auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2(size.x - s_xPad - s_xPad, size.y - s_dy), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(pos.x + s_xPad, pos.y + s_dy), ImGuiCond_Always);
	if ((ret = ImGui::Begin(id.data(), nullptr, flags))) {
		if (auto tab = TabBar(id)) { loopItems(scene); }
	}
	ImGui::End();
#endif
	return ret;
}

void PaletteTab::loopItems(MU SceneRef scene) {
#if defined(LEVK_USE_IMGUI)
	for (auto& item : m_items) {
		if (auto it = TabBar::Item(item.id)) { item.palette->doUpdate(scene); }
	}
#endif
}
} // namespace le::edi
