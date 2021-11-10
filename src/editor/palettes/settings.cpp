#include <core/services.hpp>
#include <engine/editor/editor.hpp>
#include <engine/editor/palettes/settings.hpp>
#include <engine/engine.hpp>
#include <ktl/enum_flags/enumerate_enum.hpp>

namespace le::edi {
#if defined(LEVK_USE_IMGUI)
namespace {
void swapchain(graphics::RenderContext& rc, graphics::Vsync vsync) {
	Text txt("Swapchain");
	auto const& vn = graphics::vsyncNames;
	ktl::fixed_vector<std::string_view, std::size_t(graphics::Vsync::eCOUNT_)> vsyncNames;
	for (auto const vs : ktl::enumerate_enum<graphics::Vsync>()) {
		if (rc.supported(vs)) { vsyncNames.push_back(vn[vs]); }
	}
	Combo vsyncCombo("vsync", vsyncNames, vn[vsync]);
	if (vsyncCombo.selected != vn[vsync] && vsyncCombo.select >= 0) { rc.reconstruct(static_cast<graphics::Vsync>(vsyncCombo.select)); }
}

void renderer(graphics::ARenderer& rd) {
	Text txt("Renderer");
	f32 rs = rd.renderScale();
	TWidget<f32> rsw("Render Scale", rs, 0.03f, 75.0f, {0.5f, 4.0f});
	rd.renderScale(rs);
}
} // namespace
#endif

void Settings::update() {
#if defined(LEVK_USE_IMGUI)
	auto eng = Services::get<Engine>();
	swapchain(eng->gfx().context, eng->gfx().boot.swapchain.vsync());
	renderer(eng->gfx().context.renderer());
#endif
}
} // namespace le::edi
