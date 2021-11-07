#include <core/services.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/engine.hpp>
#include <engine/physics/collision.hpp>
#include <engine/scene/scene_registry.hpp>

namespace le {
void SceneRegistry::attach(dens::entity entity, DrawLayer layer, PropProvider provider) {
	m_registry.attach<DrawLayer>(entity, layer);
	m_registry.attach<PropProvider>(entity, provider);
}

void SceneRegistry::attach(dens::entity entity, DrawLayer layer) { m_registry.attach<DrawLayer>(entity, layer); }

dens::entity SceneRegistry::spawnNode(std::string name) {
	auto ret = m_registry.make_entity(std::move(name));
	auto& node = m_registry.attach<SceneNode>(ret, &m_root);
	node.entity(ret);
	return ret;
}

dens::entity SceneRegistry::spawnProp(std::string name, Hash layerID, PropProvider provider) {
	auto ret = spawnNode(std::move(name));
	attach(ret, layer(layerID), provider);
	return ret;
};

dens::entity SceneRegistry::spawnMesh(std::string name, Hash meshID, Hash layerID, Material material) {
	return spawnProp(std::move(name), layerID, PropProvider(meshID, material));
}

DrawLayer SceneRegistry::layer(Hash id) const {
	if (auto store = Services::find<AssetStore>()) {
		if (auto layer = store->find<DrawLayer>(id)) { return *layer; }
	}
	return {};
}

void SceneRegistry::update() {
	auto eng = Services::get<Engine>();
	for (auto [_, c] : m_registry.view<gui::ViewStack>()) {
		auto& [stack] = c;
		eng->update(stack);
	}
	for (auto [_, c] : m_registry.view<Collision>()) {
		auto& [collision] = c;
		collision.update();
	}
}

SceneRegistry2::SceneRegistry2() {
	m_root = m_registry.make_entity();
	m_registry.attach<SceneNode2>(m_root, m_root);
}

void SceneRegistry2::attach(dens::entity entity, DrawLayer layer, PropProvider provider) {
	m_registry.attach<DrawLayer>(entity, layer);
	m_registry.attach<PropProvider>(entity, provider);
}

void SceneRegistry2::attach(dens::entity entity, DrawLayer layer) { m_registry.attach<DrawLayer>(entity, layer); }

dens::entity SceneRegistry2::spawnNode(std::string name) {
	auto ret = m_registry.make_entity(std::move(name));
	auto& node = m_registry.attach<SceneNode2>(ret, ret);
	[[maybe_unused]] bool const b = node.parent(m_registry, m_root);
	EXPECT(b);
	return ret;
}

dens::entity SceneRegistry2::spawnProp(std::string name, Hash layerID, PropProvider provider) {
	auto ret = spawnNode(std::move(name));
	attach(ret, layer(layerID), provider);
	return ret;
};

dens::entity SceneRegistry2::spawnMesh(std::string name, Hash meshID, Hash layerID, Material material) {
	return spawnProp(std::move(name), layerID, PropProvider(meshID, material));
}

DrawLayer SceneRegistry2::layer(Hash id) const {
	if (auto store = Services::find<AssetStore>()) {
		if (auto layer = store->find<DrawLayer>(id)) { return *layer; }
	}
	return {};
}

void SceneRegistry2::update() {
	if (m_cleanOnUpdate) {
		for (auto [_, c] : m_registry.view<SceneNode2>()) {
			auto& [node] = c;
			node.clean(m_registry);
		}
	}
	auto eng = Services::get<Engine>();
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
