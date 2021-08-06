#include <engine/editor/main_menu.hpp>

#if defined(LEVK_USE_IMGUI)
#include <core/services.hpp>
#include <core/utils/string.hpp>
#include <engine/editor/editor.hpp>
#include <engine/engine.hpp>
#endif

namespace le::edi {
#if defined(LEVK_USE_IMGUI)
namespace {
void showDemo() {
	if (auto imgui = DearImGui::inst()) { imgui->m_showDemo = true; }
}

bool g_showStats = false;
void showStats() {
	if (auto eng = Services::locate<Engine>()) {
		if (auto p = Pane("Engine Stats", {200.0f, 250.0f}, {200.0f, 200.0f}, &g_showStats, false)) {
			auto const& s = eng->stats();
			auto t = Text(fmt::format("FPS: {}", s.frame.rate));
			t = Text(fmt::format("Frame #: {}", s.frame.count));
			t = Text(fmt::format("Uptime: {}", time::format(s.upTime)));
			auto const [bsize, bunit] = utils::friendlySize(s.gfx.bytes.buffers);
			auto const [isize, iunit] = utils::friendlySize(s.gfx.bytes.images);
			t = Text(fmt::format("Buffers: {:.1f}{}", bsize, bunit));
			t = Text(fmt::format("Images: {:.1f}{}", isize, iunit));
			t = Text(fmt::format("Draw calls: {}", s.gfx.drawCalls));
			t = Text(fmt::format("Triangles: {}", s.gfx.triCount));
			t = Text(fmt::format("Window: {}x{}", s.gfx.extents.window.x, s.gfx.extents.window.y));
			t = Text(fmt::format("Swapchain: {}x{}", s.gfx.extents.swapchain.x, s.gfx.extents.swapchain.y));
			t = Text(fmt::format("Renderer: {}x{}", s.gfx.extents.renderer.x, s.gfx.extents.renderer.y));
			Styler st(Style::eSeparator);
			auto& renderer = Services::locate<Engine>()->gfx().context.renderer();
			f32 rs = renderer.renderScale();
			TWidget<f32> rsw("Render Scale", rs, 0.03f, 75.0f, {0.5f, 4.0f});
			renderer.renderScale(rs);
		}
	}
}
} // namespace
#endif

MainMenu::MainMenu() {
#if defined(LEVK_USE_IMGUI)
	MenuList::Tree main;
	main.m_t.id = "File";
	MenuList::Menu stats{"Show Stats", []() { g_showStats = true; }};
	MenuList::Menu demo{"Show ImGui Demo", []() { showDemo(); }};
	MenuList::Menu close{"Close Editor", []() { Services::locate<Engine>()->editor().engage(false); }, true};
	MenuList::Menu quit{"Quit", []() { Services::locate<Engine>()->window().close(); }};
	main.push_front(quit);
	main.push_front(close);
	main.push_front(demo);
	main.push_front(stats);
	m_main.trees.push_back(std::move(main));
#endif
}

void MainMenu::operator()([[maybe_unused]] MenuList const& extras) const {
#if defined(LEVK_USE_IMGUI)
	if (ImGui::BeginMainMenuBar()) {
		for (auto const& tree : m_main.trees) { MenuBar::walk(tree); }
		for (auto const& tree : extras.trees) { MenuBar::walk(tree); }
		ImGui::EndMainMenuBar();
	}
	if (g_showStats) { showStats(); }
#endif
}
} // namespace le::edi
