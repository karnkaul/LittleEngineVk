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
	auto const& frame = m_renderer->render(m_switcher.get_active_scene());
	Engine::self().render(frame);
}

auto SceneManager::set_renderer(std::unique_ptr<SceneRenderer> renderer) -> void {
	if (!renderer) { return; }
	m_renderer = std::move(renderer);
}
} // namespace le
