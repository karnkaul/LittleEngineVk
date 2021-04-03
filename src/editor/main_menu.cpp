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
	MenuList::Tree main;
	main.m_t.id = "File";
	MenuList::Menu exit{"Close Editor", []() { Editor::s_engaged = false; }, true};
	MenuList::Menu demo{"Show ImGui Demo", []() { showDemo(); }};
	main.push_front(exit);
	main.push_front(demo);
	m_main.trees.push_back(std::move(main));
#endif
}

void MainMenu::operator()([[maybe_unused]] MenuList const& extras) const {
#if defined(LEVK_USE_IMGUI)
	if (ImGui::BeginMainMenuBar()) {
		for (auto const& tree : m_main.trees) {
			MenuBar::walk(tree);
		}
		for (auto const& tree : extras.trees) {
			MenuBar::walk(tree);
		}
		ImGui::EndMainMenuBar();
	}
#endif
}
} // namespace le::edi
