#include <levk/engine/scene/scene_manager.hpp>

namespace le {
SceneManager::~SceneManager() { close(); }

bool SceneManager::open(Hash id) {
	if (auto it = m_scenes.find(id); it != m_scenes.end()) {
		close();
		m_active = &it->second;
		m_active->scene->open();
		logI(LC_EndUser, "[Scene] [{}] opened", m_active->id);
		return true;
	}
	return false;
}

void SceneManager::tick(Time_s dt) {
	if (m_active) { m_active->scene->tick(dt); }
}

void SceneManager::render(graphics::RGBA clear) {
	if (auto rp = m_engine.beginRenderPass(this, clear)) {
		if (m_active) { m_active->scene->render(*rp); }
		m_engine.endRenderPass(*rp);
	}
}

void SceneManager::close() {
	if (m_active) {
		m_active->scene->close();
		logI(LC_EndUser, "[Scene] [{}] closed", m_active->id);
		m_active = {};
	}
}
} // namespace le
