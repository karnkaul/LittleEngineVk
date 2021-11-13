#include <core/services.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/editor/scene_ref.hpp>
#include <engine/engine.hpp>
#include <engine/physics/collision.hpp>
#include <engine/scene/scene_registry.hpp>

namespace le {
namespace {
template <typename... Types>
dens::entity_view<Transform, SceneNode, Types...> makeNode(dens::registry& out, std::string name = {}) {
	auto e = out.make_entity<Transform, Types...>(std::move(name));
	auto& node = out.attach<SceneNode>(e, e);
	return {e, std::tie(out.get<Transform>(e), node, out.get<Types>(e)...)};
}
} // namespace

SceneRegistry::SceneRegistry() { m_root = makeNode(m_registry); }

void SceneRegistry::attach(dens::entity entity, DrawLayer layer, PropProvider provider) {
	m_registry.attach<DrawLayer>(entity, layer);
	m_registry.attach<PropProvider>(entity, provider);
}

void SceneRegistry::attach(dens::entity entity, DrawLayer layer) { m_registry.attach<DrawLayer>(entity, layer); }

dens::entity SceneRegistry::spawnNode(std::string name) {
	auto ret = makeNode(m_registry, std::move(name));
	[[maybe_unused]] bool const b = ret.get<SceneNode>().parent(m_registry, m_root);
	EXPECT(b);
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
	if (m_cleanOnUpdate) {
		for (auto [_, c] : m_registry.view<SceneNode>()) {
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

edi::SceneRef SceneRegistry::ediScene() noexcept { return {m_registry, m_root}; }
} // namespace le
