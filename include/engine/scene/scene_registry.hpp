#pragma once
#include <core/utils/vbase.hpp>
#include <dens/registry.hpp>
#include <engine/gui/view.hpp>
#include <engine/scene/draw_list_factory.hpp>
#include <engine/scene/prop_provider.hpp>
#include <engine/scene/scene_node.hpp>
#include <engine/scene/skybox.hpp>

namespace le {
namespace edi {
class Inspector;
}

class SceneRegistry : public utils::VBase {
  public:
	dens::registry& registry() noexcept { return m_registry; }
	dens::registry const& registry() const noexcept { return m_registry; }
	SceneNode::Root& root() noexcept { return m_root; }
	SceneNode::Root const& root() const noexcept { return m_root; }

	void attach(dens::entity entity, DrawLayer layer, PropProvider provider);
	void attach(dens::entity entity, DrawLayer layer);

	dens::entity spawnNode(std::string name);
	dens::entity spawnProp(std::string name, Hash layerID, PropProvider provider);
	dens::entity spawnMesh(std::string name, Hash meshID, Hash layerID, Material material = {});

	template <typename T>
	dens::entity spawnProp(std::string name, Hash assetID, Hash layerID);
	template <typename T>
	dens::entity spawnProp(std::string name, T const& source, Hash layerID);

	template <typename T, typename... Args>
	dens::entity spawn(std::string name, Hash layerID, Args&&... args);

	// Updates gui::ViewStack and Collision instances
	void update();
	DrawLayer layer(Hash id) const;

  protected:
	SceneNode::Root m_root;
	dens::registry m_registry;
};

class SceneRegistry2 : public utils::VBase {
  public:
	SceneRegistry2();

	dens::registry const& registry() const noexcept { return m_registry; }
	dens::entity root() const noexcept { return m_root; }

	void attach(dens::entity entity, DrawLayer layer, PropProvider provider);
	void attach(dens::entity entity, DrawLayer layer);

	dens::entity spawnNode(std::string name);
	dens::entity spawnProp(std::string name, Hash layerID, PropProvider provider);
	dens::entity spawnMesh(std::string name, Hash meshID, Hash layerID, Material material = {});

	template <typename T>
	dens::entity spawnProp(std::string name, Hash assetID, Hash layerID);
	template <typename T>
	dens::entity spawnProp(std::string name, T const& source, Hash layerID);

	template <typename T, typename... Args>
	dens::entity spawn(std::string name, Hash layerID, Args&&... args);

	// Updates gui::ViewStack and Collision instances
	void update();
	DrawLayer layer(Hash id) const;

	bool m_cleanOnUpdate = levk_debug;

  protected:
	dens::registry m_registry;
	dens::entity m_root;

	friend class edi::Inspector;
};

// impl

template <typename T>
dens::entity SceneRegistry::spawnProp(std::string name, T const& source, Hash layerID) {
	return spawnProp(std::move(name), layerID, PropProvider::make<T>(source));
}

template <typename T>
dens::entity SceneRegistry::spawnProp(std::string name, Hash assetID, Hash layerID) {
	if constexpr (std::is_same_v<T, graphics::Mesh>) {
		return spawnMesh(std::move(name), assetID, layerID);
	} else {
		return spawnProp(std::move(name), layerID, PropProvider::make<T>(assetID));
	}
}

template <typename T, typename... Args>
dens::entity SceneRegistry::spawn(std::string name, Hash layerID, Args&&... args) {
	auto ret = m_registry.make_entity(std::move(name));
	auto& t = m_registry.attach<T>(ret, std::forward<Args>(args)...);
	m_registry.attach<DrawLayer>(ret, layer(layerID));
	return ret;
}

template <typename T>
dens::entity SceneRegistry2::spawnProp(std::string name, T const& source, Hash layerID) {
	return spawnProp(std::move(name), layerID, PropProvider::make<T>(source));
}

template <typename T>
dens::entity SceneRegistry2::spawnProp(std::string name, Hash assetID, Hash layerID) {
	if constexpr (std::is_same_v<T, graphics::Mesh>) {
		return spawnMesh(std::move(name), assetID, layerID);
	} else {
		return spawnProp(std::move(name), layerID, PropProvider::make<T>(assetID));
	}
}

template <typename T, typename... Args>
dens::entity SceneRegistry2::spawn(std::string name, Hash layerID, Args&&... args) {
	auto ret = m_registry.make_entity(std::move(name));
	auto& t = m_registry.attach<T>(ret, std::forward<Args>(args)...);
	m_registry.attach<DrawLayer>(ret, layer(layerID));
	return ret;
}
} // namespace le
