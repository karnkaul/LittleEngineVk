#pragma once
#include <core/utils/vbase.hpp>
#include <dumb_ecf/registry.hpp>
#include <engine/gui/view.hpp>
#include <engine/scene/draw_list_factory.hpp>
#include <engine/scene/scene_node.hpp>
#include <engine/scene/skybox.hpp>

namespace le {
class SceneRegistry : public utils::VBase {
  public:
	decf::registry& registry() noexcept { return m_registry; }
	decf::registry const& registry() const noexcept { return m_registry; }
	SceneNode::Root& root() noexcept { return m_root; }
	SceneNode::Root const& root() const noexcept { return m_root; }

	void attach(decf::entity entity, DrawLayer layer, Span<Primitive const> primitives);
	void attach(decf::entity entity, DrawLayer layer);
	decf::spawn_t<SceneNode> spawnNode(std::string name);
	decf::spawn_t<SceneNode> spawn(std::string name, DrawLayer layer, not_null<graphics::Mesh const*> mesh, Material const& material);
	decf::spawn_t<SceneNode> spawn(std::string name, DrawLayer layer, Span<Primitive const> primitives);
	decf::spawn_t<Skybox> spawnSkybox(DrawLayer layer, not_null<Skybox::Cubemap const*> cubemap);
	decf::spawn_t<gui::ViewStack> spawnStack(std::string name, DrawLayer layer, not_null<graphics::VRAM*> vram);

  protected:
	SceneNode::Root m_root;
	decf::registry m_registry;
	decf::entity m_skybox;
};

// impl

inline void SceneRegistry::attach(decf::entity entity, DrawLayer layer, Span<Primitive const> primitives) {
	DrawListFactory::attach(m_registry, entity, layer, primitives);
}

inline void SceneRegistry::attach(decf::entity entity, DrawLayer layer) { m_registry.attach<DrawLayer>(entity, layer); }

inline decf::spawn_t<SceneNode> SceneRegistry::spawnNode(std::string name) {
	auto ret = m_registry.spawn<SceneNode>(name, &m_root);
	ret.get<SceneNode>().entity(ret);
	return ret;
}

inline decf::spawn_t<SceneNode> SceneRegistry::spawn(std::string name, DrawLayer layer, not_null<graphics::Mesh const*> mesh, Material const& material) {
	auto ret = spawnNode(std::move(name));
	auto& prim = m_registry.attach<Primitive>(ret);
	prim = {material, mesh};
	attach(ret, layer, prim);
	return ret;
}

inline decf::spawn_t<SceneNode> SceneRegistry::spawn(std::string name, DrawLayer layer, Span<Primitive const> primitives) {
	auto ret = spawnNode(std::move(name));
	attach(ret, layer, primitives);
	return ret;
};

inline decf::spawn_t<Skybox> SceneRegistry::spawnSkybox(DrawLayer layer, not_null<const Skybox::Cubemap*> cubemap) {
	ensure(!m_skybox.valid(), "Duplicate skybox");
	auto ret = m_registry.spawn<Skybox>("skybox", cubemap);
	m_registry.attach<DrawLayer>(ret, layer);
	m_skybox = ret;
	return ret;
}

inline decf::spawn_t<gui::ViewStack> SceneRegistry::spawnStack(std::string name, DrawLayer layer, not_null<graphics::VRAM*> vram) {
	auto ret = m_registry.spawn<gui::ViewStack>(std::move(name), vram);
	m_registry.attach<DrawLayer>(ret, layer);
	return ret;
}
} // namespace le
