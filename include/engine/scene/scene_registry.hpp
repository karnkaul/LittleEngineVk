#pragma once
#include <core/utils/vbase.hpp>
#include <dumb_ecf/registry.hpp>
#include <engine/gui/view.hpp>
#include <engine/scene/draw_list_factory.hpp>
#include <engine/scene/prop_provider.hpp>
#include <engine/scene/scene_node.hpp>
#include <engine/scene/skybox.hpp>

namespace le {
class SceneRegistry : public utils::VBase {
  public:
	decf::registry& registry() noexcept { return m_registry; }
	decf::registry const& registry() const noexcept { return m_registry; }
	SceneNode::Root& root() noexcept { return m_root; }
	SceneNode::Root const& root() const noexcept { return m_root; }

	void attach(decf::entity entity, DrawLayer layer, PropProvider provider);
	void attach(decf::entity entity, DrawLayer layer);

	decf::spawn_t<SceneNode> spawnNode(std::string name);
	decf::spawn_t<SceneNode> spawnProp(std::string name, Hash layerID, PropProvider provider);
	decf::spawn_t<SceneNode> spawnMesh(std::string name, Hash meshID, Hash layerID, Material material = {});

	template <typename T>
	decf::spawn_t<SceneNode> spawnProp(std::string name, Hash assetID, Hash layerID);
	template <typename T>
	decf::spawn_t<SceneNode> spawnProp(std::string name, T const& source, Hash layerID);

	template <typename T, typename... Args>
	decf::spawn_t<T> spawn(std::string name, Hash layerID, Args&&... args);

	DrawLayer layer(Hash id) const;

  protected:
	SceneNode::Root m_root;
	decf::registry m_registry;
};

// impl

template <typename T>
decf::spawn_t<SceneNode> SceneRegistry::spawnProp(std::string name, T const& source, Hash layerID) {
	return spawnProp(std::move(name), layerID, PropProvider::make<T>(source));
}

template <typename T>
decf::spawn_t<SceneNode> SceneRegistry::spawnProp(std::string name, Hash assetID, Hash layerID) {
	if constexpr (std::is_same_v<T, graphics::Mesh>) {
		return spawnMesh(std::move(name), assetID, layerID);
	} else {
		return spawnProp(std::move(name), layerID, PropProvider::make<T>(assetID));
	}
}

template <typename T, typename... Args>
decf::spawn_t<T> SceneRegistry::spawn(std::string name, Hash layerID, Args&&... args) {
	auto ret = m_registry.spawn<T>(std::move(name), std::forward<Args>(args)...);
	m_registry.attach<DrawLayer>(ret, layer(layerID));
	return ret;
}
} // namespace le
