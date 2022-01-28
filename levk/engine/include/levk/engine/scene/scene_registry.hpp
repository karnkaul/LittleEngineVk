#pragma once
#include <dens/registry.hpp>
#include <levk/core/utils/vbase.hpp>
#include <levk/engine/ecs/systems/system_groups.hpp>
#include <levk/engine/gui/view.hpp>
#include <levk/engine/scene/asset_provider.hpp>
#include <levk/engine/scene/scene_node.hpp>

namespace le {
namespace graphics {
struct Camera;
}

namespace edi {
class SceneRef;
}

class SceneRegistry : public utils::VBase {
  public:
	SceneRegistry();

	dens::registry const& registry() const noexcept { return m_registry; }
	dens::entity root() const noexcept { return m_sceneRoot; }

	void attach(dens::entity entity, RenderPipeProvider&& rp);

	dens::entity spawnNode(std::string name);

	dens::entity spawnMesh(std::string name, MeshProvider&& provider, std::string layerURI);
	dens::entity spawnMesh(std::string name, DynamicMesh&& dynMesh, std::string layerURI);
	template <MeshAPI T>
	dens::entity spawnMesh(std::string name, std::string assetURI, std::string layerURI);

	template <typename T, typename... Args>
	dens::entity spawn(std::string name, std::string layerURI, Args&&... args);

	void updateSystems(Time_s dt, Engine::Service const& engine);
	Material const* defaultMaterial() const;

	edi::SceneRef ediScene() noexcept;
	graphics::Camera const& camera() const noexcept;

  protected:
	ComponentSystemGroup m_systemGroupRoot;
	dens::registry m_registry;
	dens::entity m_sceneRoot;
};

// impl

template <MeshAPI T>
dens::entity SceneRegistry::spawnMesh(std::string name, std::string assetURI, std::string layerURI) {
	return spawnMesh(std::move(name), MeshProvider::make<T>(std::move(assetURI)), std::move(layerURI));
}

template <typename T, typename... Args>
dens::entity SceneRegistry::spawn(std::string name, std::string layerURI, Args&&... args) {
	auto ret = m_registry.make_entity(std::move(name));
	auto& t = m_registry.attach<T>(ret, std::forward<Args>(args)...);
	m_registry.attach<RenderPipeProvider>(ret, RenderPipeProvider::make(std::move(layerURI)));
	return ret;
}
} // namespace le
