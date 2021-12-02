#pragma once
#include <core/utils/vbase.hpp>
#include <dens/registry.hpp>
#include <engine/ecs/systems/system_groups.hpp>
#include <engine/gui/view.hpp>
#include <engine/render/draw_list_factory.hpp>
#include <engine/render/mesh_provider.hpp>
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

	void attach(dens::entity entity, DrawGroupProvider&& group);

	dens::entity spawnNode(std::string name);

	dens::entity spawnMesh(std::string name, MeshProvider&& provider, std::string groupURI);
	dens::entity spawnMesh(std::string name, DynamicMesh&& dynMesh, std::string groupURI);
	template <MeshAPI T>
	dens::entity spawnMesh(std::string name, std::string assetURI, std::string groupURI);

	template <typename T, typename... Args>
	dens::entity spawn(std::string name, std::string groupURI, Args&&... args);

	void updateSystems(dts::scheduler& scheduler, Time_s dt, input::Frame const* frame = {});
	Material const* defaultMaterial() const;

	edi::SceneRef ediScene() noexcept;

  protected:
	ComponentSystemGroup m_systemGroupRoot;
	dens::registry m_registry;
	dens::entity m_sceneRoot;
};

// impl

template <MeshAPI T>
dens::entity SceneRegistry::spawnMesh(std::string name, std::string assetURI, std::string groupURI) {
	return spawnMesh(std::move(name), MeshProvider::make<T>(std::move(assetURI)), std::move(groupURI));
}

template <typename T, typename... Args>
dens::entity SceneRegistry::spawn(std::string name, std::string groupURI, Args&&... args) {
	auto ret = m_registry.make_entity(std::move(name));
	auto& t = m_registry.attach<T>(ret, std::forward<Args>(args)...);
	m_registry.attach<DrawGroupProvider>(ret, DrawGroupProvider::make(std::move(groupURI)));
	return ret;
}
} // namespace le
