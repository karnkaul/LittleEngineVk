#pragma once
#include <dens/registry.hpp>
#include <levk/core/utils/vbase.hpp>
#include <levk/engine/assets/asset_provider.hpp>
#include <levk/engine/render/mesh_view_provider.hpp>
#include <levk/gameplay/ecs/systems/system_groups.hpp>
#include <levk/gameplay/gui/view.hpp>
#include <levk/gameplay/scene/scene_node.hpp>

namespace le {
namespace graphics {
struct Camera;
}

namespace editor {
class SceneRef;
}

class SceneRegistry : public utils::VBase {
  public:
	SceneRegistry();

	dens::registry const& registry() const noexcept { return m_registry; }
	dens::entity root() const noexcept { return m_sceneRoot; }

	void attach(dens::entity entity, RenderPipeProvider&& rp);

	dens::entity spawnNode(std::string name);

	dens::entity spawnMesh(std::string name, MeshViewProvider&& provider, std::string pipeURI);
	dens::entity spawnMesh(std::string name, DynamicMeshView&& dynMesh, std::string pipeURI);
	template <MeshViewAPI T>
	dens::entity spawnMesh(std::string name, std::string assetURI, std::string pipeURI);

	template <typename T, typename... Args>
	dens::entity spawn(std::string name, std::string pipeURI, Args&&... args);

	void updateSystems(Time_s dt, Engine::Service const& engine);
	Material const* defaultMaterial() const;

	editor::SceneRef ediScene() noexcept;
	graphics::Camera const& camera() const noexcept;

  protected:
	ComponentSystemGroup m_systemGroupRoot;
	dens::registry m_registry;
	dens::entity m_sceneRoot;
};

// impl

template <MeshViewAPI T>
dens::entity SceneRegistry::spawnMesh(std::string name, std::string assetURI, std::string pipeURI) {
	return spawnMesh(std::move(name), MeshViewProvider::make<T>(std::move(assetURI)), std::move(pipeURI));
}

template <typename T, typename... Args>
dens::entity SceneRegistry::spawn(std::string name, std::string pipeURI, Args&&... args) {
	auto ret = m_registry.make_entity(std::move(name));
	auto& t = m_registry.attach<T>(ret, std::forward<Args>(args)...);
	m_registry.attach<RenderPipeProvider>(ret, std::move(pipeURI));
	return ret;
}
} // namespace le
