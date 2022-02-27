#include <levk/core/services.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/engine.hpp>
#include <levk/gameplay/ecs/systems/gui_system.hpp>
#include <levk/gameplay/ecs/systems/physics_system.hpp>
#include <levk/gameplay/ecs/systems/scene_clean_system.hpp>
#include <levk/gameplay/ecs/systems/spring_arm_system.hpp>
#include <levk/gameplay/ecs/systems/system_groups.hpp>
#include <levk/gameplay/editor/scene_ref.hpp>
#include <levk/gameplay/scene/scene_registry.hpp>
#include <levk/graphics/mesh.hpp>
#include <levk/graphics/mesh_primitive.hpp>
#include <levk/graphics/render/camera.hpp>

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

void SceneRegistry::attach(dens::entity entity, RenderPipeProvider&& rp) { m_registry.attach<RenderPipeProvider>(entity, std::move(rp)); }

dens::entity SceneRegistry::spawnNode(std::string name) {
	auto ret = makeNode(m_registry, std::move(name));
	[[maybe_unused]] bool const b = ret.get<SceneNode>().parent(m_registry, m_sceneRoot);
	EXPECT(b);
	return ret;
}

dens::entity SceneRegistry::spawn(std::string name, PrimitiveProvider provider, Hash renderPipeline) {
	auto ret = spawnNode(std::move(name));
	m_registry.attach(ret, RenderPipeProvider(renderPipeline));
	m_registry.attach(ret, std::move(provider));
	return ret;
}

dens::entity SceneRegistry::spawn(std::string name, AssetProvider<graphics::Mesh> provider, Hash renderPipeline) {
	auto ret = spawnNode(std::move(name));
	m_registry.attach(ret, RenderPipeProvider(renderPipeline));
	m_registry.attach(ret, std::move(provider));
	return ret;
}

void SceneRegistry::updateSystems(Time_s dt, Engine::Service const& engine) { m_systemGroupRoot.update(m_registry, SystemData{engine, dt}); }

editor::SceneRef SceneRegistry::ediScene() noexcept { return {m_registry, m_sceneRoot}; }
graphics::Camera const& SceneRegistry::camera() const noexcept { return m_registry.get<graphics::Camera>(m_sceneRoot); }
} // namespace le
