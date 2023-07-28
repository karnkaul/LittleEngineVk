#include <spaced/engine.hpp>
#include <spaced/scene/scene_manager.hpp>

namespace spaced {
auto SceneManager::tick(Duration dt) -> void {
	if (m_switcher.m_standby) {
		m_switcher.m_active = std::move(m_switcher.m_standby);
		m_switcher.m_active->setup();
		// reset dt, it isn't valid for a new scene
		dt = {};
	}

	m_switcher.m_active->tick(dt);
}

auto SceneManager::set_clear_colour(graphics::Rgba clear) -> void {
	auto const rgba = graphics::Rgba::to_linear(clear.to_tint());
	m_renderer->clear_colour = std::array{rgba.x, rgba.y, rgba.z, rgba.w};
}

auto SceneManager::render() const -> void {
	auto subpasses = std::array<Ptr<graphics::RenderPass>, 1>{m_renderer.get()};
	Engine::self().render(subpasses);
}

auto SceneManager::render(std::span<Ptr<graphics::RenderPass> const> subpasses) const -> void {
	auto render_passes = std::vector<Ptr<graphics::RenderPass>>{};
	render_passes.reserve(subpasses.size() + 1);
	render_passes.push_back(m_renderer.get());
	render_passes.insert(render_passes.end(), subpasses.begin(), subpasses.end());
	Engine::self().render(render_passes);
}
} // namespace spaced
