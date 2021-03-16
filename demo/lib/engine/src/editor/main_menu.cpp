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
	edi::Menu::Item exit{"Close Editor", []() { Editor::s_engaged = false; }, {}, true};
	edi::Menu::Item demo{"Show ImGui Demo", []() { showDemo(); }, {}, false};
	edi::Menu file{"File", {demo, exit}};
	m_main.push_back(std::move(file));
#endif
}

void MainMenu::operator()([[maybe_unused]] Span<Menu> extras) const {
#if defined(LEVK_USE_IMGUI)
	auto copy = m_main;
	for (Menu& extra : extras) {
		if (!extra.id.empty()) {
			for (Menu& menu : copy) {
				if (menu.id == extra.id) {
					std::move(extra.items.begin(), extra.items.end(), std::back_inserter(menu.items));
					extra = {};
					break;
				}
			}
		}
	}
	for (Menu& extra : extras) {
		if (!extra.id.empty()) {
			copy.push_back(std::move(extra));
		}
	}
	if (ImGui::BeginMainMenuBar()) {
		for (auto const& menu : copy) {
			menu.walk();
		}
		ImGui::EndMainMenuBar();
	}
#endif
}
} // namespace le::edi
