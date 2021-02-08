#include <engine/game/scene.hpp>

namespace le {
using namespace ecs;

gfx::Camera const& GameScene::Desc::camera() const {
	return pCustomCam ? *pCustomCam : defaultCam;
}

gfx::Camera& GameScene::Desc::camera() {
	return pCustomCam ? *pCustomCam : defaultCam;
}

void GameScene::reset() {
	Registry& reg = m_registry;
	reg.clear();
	m_name.clear();
#if defined(LEVK_EDITOR)
	m_editorData = {};
#endif
}

GameScene::Desc& GameScene::desc() {
	Registry& reg = m_registry;
	auto view = reg.view<Desc>();
	auto [_, desc] = view.empty() ? reg.spawn<Desc>("scene_desc") : *view.begin();
	auto& [desc_] = desc;
	return desc_;
}

gfx::Camera& GameScene::mainCamera() {
	return desc().camera();
}

bool GameScene::reparent(Prop prop, Transform* pParent) {
	if (prop.valid()) {
		if constexpr (s_bParentToRoot) {
			if (!pParent) {
				pParent = &m_sceneRoot;
			}
		}
		prop.transform().parent(pParent);
		return true;
	}
	return false;
}

void GameScene::destroy(View<Prop> props) {
	Registry& reg = m_registry;
	for (auto& prop : props) {
		if (prop.pTransform) {
			m_entityMap.erase(*prop.pTransform);
		}
		reg.destroy(prop.entity);
	}
}

void GameScene::boltOnRoot(Prop prop) {
	auto& transform = prop.transform();
	transform.parent(&m_sceneRoot);
	m_entityMap[transform] = prop.entity;
}
} // namespace le
