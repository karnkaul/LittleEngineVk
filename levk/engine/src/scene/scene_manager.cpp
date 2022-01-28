#include <dumb_tasks/executor.hpp>
#include <levk/engine/render/frame.hpp>
#include <levk/engine/render/shader_data.hpp>
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
	if (m_active) {
		auto p = m_engine.profile("tick");
		m_active->scene->tick(dt);
	}
}

void SceneManager::render(graphics::RGBA clear) {
	auto p = m_engine.profile("render");
	if (auto frame = RenderFrame(sceneRef(), m_engine, clear)) {
		if (m_active) {
			auto const view = ShaderSceneView::make(m_active->scene->camera(), m_engine.sceneSpace());
			m_active->scene->render(frame.renderPass(), view);
		}
		m_shaderBufferMap.swap();
	}
}

void SceneManager::close() {
	m_engine.executor().stop();
	if (m_active) {
		m_active->scene->close();
		logI(LC_EndUser, "[Scene] [{}] closed", m_active->id);
		m_active = {};
	}
}
} // namespace le
