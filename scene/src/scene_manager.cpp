#include <le/core/logger.hpp>
#include <le/engine.hpp>
#include <le/graphics/cache/vertex_buffer_cache.hpp>
#include <le/scene/scene_manager.hpp>

namespace le {
namespace {
auto const g_log{logger::Logger{"SceneManager"}};
}

auto SceneManager::tick(Duration dt) -> void {
	if (m_switcher.m_standby) {
		g_log.debug("Switching Scene...");
		m_switcher.m_active = std::move(m_switcher.m_standby);
		g_log.debug("Clearing resources...");
		Resources::self().clear();
		g_log.debug("Clearing Vertex Buffer Cache...");
		graphics::VertexBufferCache::self().clear();
		g_log.debug("Setting up Scene...");
		m_switcher.m_active->setup();
		g_log.debug("... Scene switched");
		// reset dt, it isn't valid for a new scene
		dt = {};
	}

	m_switcher.m_active->tick(dt);
}

auto SceneManager::render() const -> void {
	m_renderer->scene = &m_switcher.get_active_scene();
	auto subpasses = std::array<NotNull<graphics::Subpass*>, 1>{m_renderer.get()};
	Engine::self().render(subpasses);
}

auto SceneManager::render(std::span<Ptr<graphics::Subpass> const> subpasses) const -> void {
	auto subass_ptrs = std::vector<NotNull<graphics::Subpass*>>{};
	subass_ptrs.reserve(subpasses.size() + 2);
	subass_ptrs.emplace_back(m_renderer.get());
	subass_ptrs.insert(subass_ptrs.end(), subpasses.begin(), subpasses.end());
	Engine::self().render(subass_ptrs);
}

auto SceneManager::set_renderer(std::unique_ptr<SceneRenderer> renderer) -> void {
	if (!renderer) { return; }
	m_renderer = std::move(renderer);
}
} // namespace le
