#include <core/services.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/ecs/systems/gui_system.hpp>
#include <engine/ecs/systems/physics_system.hpp>
#include <engine/ecs/systems/scene_clean_system.hpp>
#include <engine/ecs/systems/spring_arm_system.hpp>
#include <engine/ecs/systems/system_groups.hpp>
#include <engine/editor/scene_ref.hpp>
#include <engine/engine.hpp>
#include <engine/scene/scene_registry.hpp>
#include <graphics/mesh_primitive.hpp>
#include <graphics/render/camera.hpp>

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
	m_sceneRoot = makeNode(m_registry, "scene_root");
	m_registry.attach<graphics::Camera>(m_sceneRoot);
	auto& physics = m_systemGroupRoot.attach<PhysicsSystemGroup>();
	physics.attach<PhysicsSystem>();
	auto& tick = m_systemGroupRoot.attach<TickSystemGroup>();
	tick.attach<SpringArmSystem>();
	tick.attach<GuiSystem>(GuiSystem::order_v);
	tick.attach<SceneCleanSystem>(SceneCleanSystem::order_v);
}

void SceneRegistry::attach(dens::entity entity, DrawGroup group, PropProvider provider) {
	m_registry.attach<DrawGroup>(entity, group);
	m_registry.attach<PropProvider>(entity, provider);
}

void SceneRegistry::attach(dens::entity entity, DrawGroup group) { m_registry.attach<DrawGroup>(entity, group); }

dens::entity SceneRegistry::spawnNode(std::string name) {
	auto ret = makeNode(m_registry, std::move(name));
	[[maybe_unused]] bool const b = ret.get<SceneNode>().parent(m_registry, m_sceneRoot);
	EXPECT(b);
	return ret;
}

dens::entity SceneRegistry::spawnProp(std::string name, Hash groupURI, PropProvider provider) {
	auto ret = spawnNode(std::move(name));
	attach(ret, drawGroup(groupURI), provider);
	return ret;
};

dens::entity SceneRegistry::spawnMesh(std::string name, Hash meshURI, Hash groupURI, Material material) {
	return spawnProp(std::move(name), groupURI, PropProvider(meshURI, material));
}

DrawGroup SceneRegistry::drawGroup(Hash id) const {
	if (auto store = Services::find<AssetStore>()) {
		if (auto group = store->find<DrawGroup>(id)) { return *group; }
	}
	return {};
}

void SceneRegistry::updateSystems(dts::scheduler& scheduler, Time_s dt, input::Frame const* frame) {
	static input::Frame const s_blank{};
	if (!frame) {
		if (auto eng = Services::find<Engine>()) {
			frame = &eng->inputFrame();
		} else {
			frame = &s_blank;
		}
	}
	m_systemGroupRoot.update(m_registry, SystemData{scheduler, *frame, dt});
}

edi::SceneRef SceneRegistry::ediScene() noexcept { return {m_registry, m_sceneRoot}; }
} // namespace le
