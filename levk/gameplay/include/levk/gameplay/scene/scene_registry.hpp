#pragma once
#include <dens/registry.hpp>
#include <levk/core/utils/vbase.hpp>
#include <levk/engine/assets/asset_provider.hpp>
#include <levk/engine/render/primitive_provider.hpp>
#include <levk/gameplay/ecs/systems/system_groups.hpp>
#include <levk/gameplay/gui/view.hpp>
#include <levk/gameplay/scene/scene_node.hpp>

namespace le {
namespace graphics {
struct Camera;
struct Mesh;
} // namespace graphics

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

	dens::entity spawn(std::string name, PrimitiveProvider provider, Hash renderPipeline);
	dens::entity spawn(std::string name, AssetProvider<graphics::Mesh> provider, Hash renderPipeline);

	template <typename T, typename... Args>
	dens::entity spawn(std::string name, Hash pipeURI, Args&&... args);

	void updateSystems(Time_s dt, Engine::Service const& engine);

	editor::SceneRef ediScene() noexcept;
	graphics::Camera const& camera() const noexcept;

  protected:
	ComponentSystemGroup m_systemGroupRoot;
	dens::registry m_registry;
	dens::entity m_sceneRoot;
};

// impl

template <typename T, typename... Args>
dens::entity SceneRegistry::spawn(std::string name, Hash pipeURI, Args&&... args) {
	auto ret = m_registry.make_entity(std::move(name));
	auto& t = m_registry.attach<T>(ret, std::forward<Args>(args)...);
	m_registry.attach(ret, RenderPipeProvider(pipeURI));
	return ret;
}
} // namespace le
