#include <core/services.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/editor/scene_ref.hpp>
#include <engine/engine.hpp>
#include <engine/scene/scene_registry.hpp>
#include <engine/systems/gui_system.hpp>
#include <engine/systems/physics_system.hpp>
#include <engine/systems/scene_clean_system.hpp>
#include <engine/systems/spring_arm_system.hpp>
#include <engine/systems/system_groups.hpp>

namespace le {
namespace {
template <typename... Types>
dens::entity_view<Transform, SceneNode, Types...> makeNode(dens::registry& out, std::string name = {}) {
	auto e = out.make_entity<Transform, Types...>(std::move(name));
	auto& node = out.attach<SceneNode>(e, e);
	return {e, std::tie(out.get<Transform>(e), node, out.get<Types>(e)...)};
}
} // namespace

SceneRegistry::SceneRegistry() {
	m_sceneRoot = makeNode(m_registry);
	auto& physics = m_systemGroupRoot.attach<PhysicsSystemGroup>();
	physics.attach<PhysicsSystem>();
	auto& tick = m_systemGroupRoot.attach<TickSystemGroup>();
	tick.attach<SpringArmSystem>();
	tick.attach<GuiSystem>(GuiSystem::order_v);
	tick.attach<SceneCleanSystem>(SceneCleanSystem::order_v);
}

void SceneRegistry::attach(dens::entity entity, DrawLayer layer, PropProvider provider) {
	m_registry.attach<DrawLayer>(entity, layer);
	m_registry.attach<PropProvider>(entity, provider);
}

void SceneRegistry::attach(dens::entity entity, DrawLayer layer) { m_registry.attach<DrawLayer>(entity, layer); }

dens::entity SceneRegistry::spawnNode(std::string name) {
	auto ret = makeNode(m_registry, std::move(name));
	[[maybe_unused]] bool const b = ret.get<SceneNode>().parent(m_registry, m_sceneRoot);
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

void SceneRegistry::updateSystems(dts::scheduler& scheduler, Time_s dt) { m_systemGroupRoot.update(scheduler, m_registry, dt); }

edi::SceneRef SceneRegistry::ediScene() noexcept { return {m_registry, m_sceneRoot}; }
} // namespace le
