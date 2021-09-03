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
enum class Flag { eStats, eProfiler, eCOUNT_ };
static constexpr auto flag_count = static_cast<std::size_t>(Flag::eCOUNT_);

struct Panes {
	bool flags[flag_count] = {};

	bool& flag(Flag f) { return flags[static_cast<std::size_t>(f)]; }
	bool flag(Flag f) const { return flags[static_cast<std::size_t>(f)]; }

	void operator()() const;

	void showStats() const;
	template <typename T>
	void showProfiler() const;
};

Panes g_panes;

void Panes::showStats() const {
	if (auto eng = Services::get<Engine>()) {
		if (auto p = Pane("Engine Stats", {200.0f, 250.0f}, {200.0f, 200.0f}, &g_panes.flag(Flag::eStats), false)) {
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
			auto& renderer = Services::get<Engine>()->gfx().context.renderer();
			f32 rs = renderer.renderScale();
			TWidget<f32> rsw("Render Scale", rs, 0.03f, 75.0f, {0.5f, 4.0f});
			renderer.renderScale(rs);
		}
	}
}

template <typename T>
void Panes::showProfiler() const {
	if (auto profiler = Services::find<T>()) {
		if (auto p = Pane("Profiler", {600.0f, 400.0f}, {300.0f, 300.0f}, &g_panes.flag(Flag::eProfiler), false)) {
			auto const& record = profiler->m_record.back();
			Time_s const total = record.total;
			f32 maxLength{};
			for (auto const& profile : record.entries) { maxLength = std::max(maxLength, ImGui::CalcTextSize(profile.name.data()).x); }
			for (auto const& profile : record.entries) {
				Text t(profile.name);
				ImGui::SameLine(maxLength + 20.0f);
				ImGui::ProgressBar(profile.dt / total, ImVec2{-1.0f, 0.0f}, fmt::format("{:1.2f}ms", profile.dt.count() * 1000.0f).data());
			}
		}
		profiler->m_record.clear();
	}
}

template <>
void Panes::showProfiler<utils::NullProfileDB>() const {}

void Panes::operator()() const {
	if (flag(Flag::eStats)) { showStats(); }
	if (flag(Flag::eProfiler)) { showProfiler<Engine::Profiler>(); }
}
} // namespace
#endif

MainMenu::MainMenu() {
#if defined(LEVK_USE_IMGUI)
	MenuList::Tree main;
	main.m_t.id = "File";
	MenuList::Menu imDemo{"Show ImGui Demo", []() { Services::get<Engine>()->gfx().imgui.m_showDemo = true; }};
	MenuList::Menu stats{"Show Stats", []() { g_panes.flag(Flag::eStats) = true; }};
	MenuList::Menu profiler{"Show Profiler", []() { g_panes.flag(Flag::eProfiler) = true; }};
	MenuList::Menu close{"Close Editor", []() { Services::get<Engine>()->editor().engage(false); }, true};
	MenuList::Menu quit{"Quit", []() { Services::get<Engine>()->window().close(); }};
	main.push_front(quit);
	main.push_front(close);
	main.push_front(imDemo);
	main.push_front(profiler);
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
	g_panes();
#endif
}
} // namespace le::edi
