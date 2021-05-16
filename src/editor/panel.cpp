#include <engine/editor/panel.hpp>
#include <levk_imgui/levk_imgui.hpp>

namespace le::edi {
bool Palette::detach(std::string const& id) {
	if (auto it = m_items.find(id); it != m_items.end()) {
		m_items.erase(it);
		return true;
	}
	return false;
}

bool Palette::update([[maybe_unused]] std::string_view id, [[maybe_unused]] glm::vec2 size, [[maybe_unused]] glm::vec2 pos) {
	bool ret = false;
#if defined(LEVK_USE_IMGUI)
	if (size.x < s_minSize.x || size.y < s_minSize.y) { return false; }
	static constexpr f32 s_xPad = 2.0f;
	static constexpr f32 const s_dy = 2.0f;
	static constexpr auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(ImVec2(size.x - s_xPad - s_xPad, size.y - s_dy), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(pos.x + s_xPad, pos.y + s_dy), ImGuiCond_Always);
	if ((ret = ImGui::Begin(id.data(), nullptr, flags)); !m_items.empty()) {
		if (auto tab = TabBar(id)) {
			for (auto& [id, item] : m_items) {
				if (auto it = TabBar::Item(id)) { item->update(); }
			}
		}
	}
	ImGui::End();
#endif
	return ret;
}
} // namespace le::edi
