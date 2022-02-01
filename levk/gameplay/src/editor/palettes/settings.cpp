#include <ktl/enum_flags/enumerate_enum.hpp>
#include <levk/core/services.hpp>
#include <levk/engine/engine.hpp>
#include <levk/gameplay/editor/editor.hpp>
#include <levk/gameplay/editor/palettes/settings.hpp>
#include <levk/graphics/render/context.hpp>

namespace le::editor {
#if defined(LEVK_USE_IMGUI)
namespace {
void context(graphics::RenderContext& rc) {
	Text txt("Context");
	auto const& vn = graphics::vSyncNames;
	ktl::fixed_vector<std::string_view, std::size_t(graphics::VSync::eCOUNT_)> vsyncNames;
	for (auto const vs : ktl::enumerate_enum<graphics::VSync>()) {
		if (rc.supported(vs)) { vsyncNames.push_back(vn[vs]); }
	}
	auto const vsync = rc.surface().format().vsync;
	Combo vsyncCombo("vsync", vsyncNames, vn[vsync]);
	if (vsyncCombo.selected != vn[vsync] && vsyncCombo.select >= 0) {
		if (rc.recreateSwapchain({}, static_cast<graphics::VSync>(vsyncCombo.select))) {
			logI("[Editor] VSync changed to [{}]", graphics::vSyncNames[rc.surface().format().vsync]);
		} else {
			logE("[Editor] Failed to change VSync to [{}]!", vsyncNames[std::size_t(vsyncCombo.select)]);
		}
	}
}

void renderer(graphics::Renderer& rd) {
	Text txt("Renderer");
	static f32 s_newScale = rd.renderScale();
	if (Button(ktl::stack_string<16>("Scale: {.2f}", rd.renderScale())) && rd.canScale()) { Popup::open("settings_renderer_scale"); }
	if (auto popup = Popup("settings_renderer_scale")) {
		TWidget<f32> rsw("Render Scale", s_newScale, 0.03f, 75.0f, {0.5f, 4.0f});
		if (Button("Set")) {
			if (rd.renderScale(s_newScale)) {
				logI("[Editor] Renderer scale changed to [{:.2f}]", rd.renderScale());
			} else {
				logW("[Editor] Failed to change renderer scale to [{:.2f}]", s_newScale);
				s_newScale = rd.renderScale();
			}
			popup.close();
		}
	}
}
} // namespace
#endif

void Settings::update() {
#if defined(LEVK_USE_IMGUI)
	auto eng = Services::get<Engine::Service>();
	context(eng->context());
	renderer(eng->context().renderer());
#endif
}
} // namespace le::editor
