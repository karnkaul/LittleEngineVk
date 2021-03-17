#include <engine/editor/main_menu.hpp>

#if defined(LEVK_USE_IMGUI)
#include <engine/editor/editor.hpp>
#endif

namespace le::edi {
#if defined(LEVK_USE_IMGUI)
namespace {
void showDemo() {
	if (auto imgui = DearImGui::inst()) {
		imgui->m_showDemo = true;
	}
}
} // namespace
#endif

MainMenu::MainMenu() {
#if defined(LEVK_USE_IMGUI)
	MenuTree::Item exit{"Close Editor", []() { Editor::s_engaged = false; }, {}, true};
	MenuTree::Item demo{"Show ImGui Demo", []() { showDemo(); }, {}};
	MenuTree file{"File", {demo, exit}};
	m_main.push_back(std::move(file));
#endif
}

void MainMenu::operator()([[maybe_unused]] Span<MenuTree> extras) const {
#if defined(LEVK_USE_IMGUI)
	auto copy = m_main;
	for (MenuTree& extra : extras) {
		if (!extra.id.empty()) {
			for (MenuTree& menu : copy) {
				if (menu.id == extra.id) {
					std::move(extra.items.begin(), extra.items.end(), std::back_inserter(menu.items));
					extra = {};
					break;
				}
			}
		}
	}
	for (MenuTree& extra : extras) {
		if (!extra.id.empty() && !extra.items.empty()) {
			copy.push_back(std::move(extra));
		}
	}
	if (ImGui::BeginMainMenuBar()) {
		for (auto const& menu : copy) {
			MenuBar::walk(menu);
		}
		ImGui::EndMainMenuBar();
	}
#endif
}
} // namespace le::edi
