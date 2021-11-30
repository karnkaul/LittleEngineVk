#pragma once
#include <core/utils/vbase.hpp>
#include <dens/registry.hpp>
#include <engine/ecs/systems/system_groups.hpp>
#include <engine/gui/view.hpp>
#include <engine/render/draw_list_factory.hpp>
#include <engine/render/prop_provider.hpp>
#include <engine/scene/scene_node.hpp>

namespace le {
namespace edi {
class SceneRef;
}

class SceneRegistry : public utils::VBase {
  public:
	SceneRegistry();

	dens::registry const& registry() const noexcept { return m_registry; }
	dens::entity root() const noexcept { return m_sceneRoot; }

	void attach(dens::entity entity, DrawGroup group, PropProvider provider);
	void attach(dens::entity entity, DrawGroup group);

	dens::entity spawnNode(std::string name);
	dens::entity spawnProp(std::string name, Hash groupURI, PropProvider provider);
	dens::entity spawnMesh(std::string name, Hash meshURI, Hash groupURI, Material material = {});

	template <typename T>
	dens::entity spawnProp(std::string name, Hash assetURI, Hash groupURI);
	template <typename T>
	dens::entity spawnProp(std::string name, T const& source, Hash groupURI);

	template <typename T, typename... Args>
	dens::entity spawn(std::string name, Hash groupURI, Args&&... args);

	void updateSystems(dts::scheduler& scheduler, Time_s dt, input::Frame const* frame = {});
	DrawGroup drawGroup(Hash id) const;

	edi::SceneRef ediScene() noexcept;

  protected:
	ComponentSystemGroup m_systemGroupRoot;
	dens::registry m_registry;
	dens::entity m_sceneRoot;
};

// impl

template <typename T>
dens::entity SceneRegistry::spawnProp(std::string name, T const& source, Hash groupURI) {
	return spawnProp(std::move(name), groupURI, PropProvider::make<T>(source));
}

template <typename T>
dens::entity SceneRegistry::spawnProp(std::string name, Hash assetID, Hash groupURI) {
	if constexpr (std::is_same_v<T, graphics::MeshPrimitive>) {
		return spawnMesh(std::move(name), assetID, groupURI);
	} else {
		return spawnProp(std::move(name), groupURI, PropProvider::make<T>(assetID));
	}
}

template <typename T, typename... Args>
dens::entity SceneRegistry::spawn(std::string name, Hash groupURI, Args&&... args) {
	auto ret = m_registry.make_entity(std::move(name));
	auto& t = m_registry.attach<T>(ret, std::forward<Args>(args)...);
	m_registry.attach<DrawGroup>(ret, drawGroup(groupURI));
	return ret;
}
} // namespace le
