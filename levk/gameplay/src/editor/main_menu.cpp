#include <levk/gameplay/editor/main_menu.hpp>

#if defined(LEVK_USE_IMGUI)
#include <imgui.h>
#include <levk/core/services.hpp>
#include <levk/core/utils/string.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/engine.hpp>
#include <levk/engine/utils/engine_stats.hpp>
#include <levk/gameplay/editor/asset_index.hpp>
#include <levk/gameplay/editor/editor.hpp>
#include <levk/graphics/render/context.hpp>
#include <levk/window/window.hpp>
#endif

namespace le::editor {
#define MU [[maybe_unused]]

#if defined(LEVK_USE_IMGUI)
namespace {
enum class Flag { eStats, eProfiler, eAssetIndex, eCOUNT_ };
static constexpr auto flag_count = static_cast<std::size_t>(Flag::eCOUNT_);

struct Panes {
	bool flags[flag_count] = {};

	bool& flag(Flag f) { return flags[static_cast<std::size_t>(f)]; }
	bool flag(Flag f) const { return flags[static_cast<std::size_t>(f)]; }

	void operator()();

	void showStats() const;
	template <typename T>
	void showProfiler() const;
};

Panes g_panes;

void Panes::showStats() const {
	if (auto eng = Services::find<Engine::Service>()) {
		if (auto p = Pane("Engine Stats", {200.0f, 250.0f}, {200.0f, 200.0f}, &g_panes.flag(Flag::eStats))) {
			auto const& s = eng->stats();
			auto t = Text(CStr<32>("FPS: {}", s.frame.rate));
			t = Text(CStr<32>("Frame #: {}", s.frame.count));
			t = Text(CStr<32>("Uptime: {}", time::format(s.upTime).data()));
			auto const [bsize, bunit] = utils::friendlySize(s.gfx.bytes.buffers);
			auto const [isize, iunit] = utils::friendlySize(s.gfx.bytes.images);
			t = Text(CStr<32>("Buffers: {.1f}{}", bsize, bunit.data()));
			t = Text(CStr<32>("Images: {.1f}{}", isize, iunit.data()));
			t = Text(CStr<32>("Draw calls: {}", s.gfx.drawCalls));
			t = Text(CStr<32>("Triangles: {}", s.gfx.triCount));
			t = Text(CStr<32>("Window: {}x{}", s.gfx.extents.window.x, s.gfx.extents.window.y));
			t = Text(CStr<32>("Swapchain: {}x{}", s.gfx.extents.swapchain.x, s.gfx.extents.swapchain.y));
			t = Text(CStr<32>("Renderer: {}x{}", s.gfx.extents.renderer.x, s.gfx.extents.renderer.y));
			Styler st(Style::eSeparator);
			auto& renderer = eng->context().renderer();
			f32 rs = renderer.renderScale();
			TWidget<f32> rsw("Render Scale", rs, 0.03f, 75.0f, {0.5f, 4.0f});
			renderer.renderScale(rs);
		}
	}
}

template <typename T>
void Panes::showProfiler() const {
	if (auto profiler = Services::find<T>()) {
		if (auto p = Pane("Profiler", {600.0f, 400.0f}, {300.0f, 300.0f}, &g_panes.flag(Flag::eProfiler))) {
			auto const& record = profiler->m_record.back();
			Time_s const total = record.total;
			f32 maxLength{};
			for (auto const& profile : record.entries) { maxLength = std::max(maxLength, ImGui::CalcTextSize(profile.name.data()).x); }
			for (auto const& profile : record.entries) {
				Text t(profile.name);
				ImGui::SameLine(maxLength + 20.0f);
				ImGui::ProgressBar(profile.dt / total, ImVec2{-1.0f, 0.0f}, CStr<16>("{1.2f}ms", profile.dt.count() * 1000.0f).data());
			}
		}
		profiler->m_record.clear();
	}
}

template <>
MU void Panes::showProfiler<utils::NullProfileDB>() const {}

void Panes::operator()() {
	if (flag(Flag::eStats)) { showStats(); }
	if (flag(Flag::eProfiler)) { showProfiler<Engine::Profiler>(); }
	if (flag(Flag::eAssetIndex)) {
		if (auto p = Pane("Asset Index", {425.0f, 250.0f}, {50.0f, 100.0f}, &flag(Flag::eAssetIndex))) {
			if (auto store = Services::find<AssetStore>()) { AssetIndex::list(*store); }
		}
	}
}
} // namespace
#endif

MainMenu::MainMenu() {
#if defined(LEVK_USE_IMGUI)
	MenuList::Tree main;
	main.m_t.id = "File";
	MenuList::Menu assetIndex{"Asset Index", []() { g_panes.flag(Flag::eAssetIndex) = true; }};
	MenuList::Menu stats{"Show Stats", []() { g_panes.flag(Flag::eStats) = true; }};
	MenuList::Menu profiler{"Show Profiler", []() { g_panes.flag(Flag::eProfiler) = true; }};
	MenuList::Menu imDemo{"Show ImGui Demo", []() { editor::showImGuiDemo(); }};
	MenuList::Menu close{"Close Editor", []() { editor::engage(false); }, true};
	MenuList::Menu quit{"Quit", []() { Services::get<Engine::Service>()->window().close(); }};
	main.push_front(std::move(quit));
	main.push_front(std::move(close));
	main.push_front(std::move(imDemo));
	main.push_front(std::move(profiler));
	main.push_front(std::move(stats));
	main.push_front(std::move(assetIndex));
	m_main.trees.push_back(std::move(main));
#endif
}

void MainMenu::operator()(MU MenuList const& extras) const {
#if defined(LEVK_USE_IMGUI)
	if (ImGui::BeginMainMenuBar()) {
		for (auto const& tree : m_main.trees) { MenuBar::walk(tree); }
		for (auto const& tree : extras.trees) { MenuBar::walk(tree); }
		ImGui::EndMainMenuBar();
	}
	g_panes();
#endif
}
} // namespace le::editor
