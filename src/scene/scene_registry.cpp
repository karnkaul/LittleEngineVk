#include <core/services.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/engine.hpp>
#include <engine/physics/collision.hpp>
#include <engine/scene/scene_registry.hpp>

namespace le {
void SceneRegistry::attach(decf::entity entity, DrawLayer layer, PropProvider provider) {
	m_registry.attach<DrawLayer>(entity, layer);
	m_registry.attach<PropProvider>(entity, provider);
}

void SceneRegistry::attach(decf::entity entity, DrawLayer layer) { m_registry.attach<DrawLayer>(entity, layer); }

decf::spawn_t<SceneNode> SceneRegistry::spawnNode(std::string name) {
	auto ret = m_registry.spawn<SceneNode>(name, &m_root);
	ret.get<SceneNode>().entity(ret);
	return ret;
}

decf::spawn_t<SceneNode> SceneRegistry::spawnProp(std::string name, Hash layerID, PropProvider provider) {
	auto ret = spawnNode(std::move(name));
	attach(ret, layer(layerID), provider);
	return ret;
};

decf::spawn_t<SceneNode> SceneRegistry::spawnMesh(std::string name, Hash meshID, Hash layerID, Material material) {
	return spawnProp(std::move(name), layerID, PropProvider(meshID, material));
}

DrawLayer SceneRegistry::layer(Hash id) const {
	if (auto store = Services::locate<AssetStore>(false)) {
		if (auto layer = store->find<DrawLayer>(id)) { return *layer; }
	}
	return {};
}

void SceneRegistry::update() {
	auto eng = Services::locate<Engine>();
	for (auto [_, c] : m_registry.view<gui::ViewStack>()) {
		auto& [stack] = c;
		eng->update(stack);
	}
	for (auto [_, c] : m_registry.view<Collision>()) {
		auto& [collision] = c;
		collision.update();
	}
}
} // namespace le