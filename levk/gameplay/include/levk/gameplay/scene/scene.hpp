#pragma once
#include <levk/engine/engine.hpp>
#include <levk/gameplay/scene/scene_registry.hpp>

namespace le {
namespace graphics {
class RenderPass;
class ShaderBuffer;
} // namespace graphics

struct ShaderSceneView;
class ShaderBufferMap;

class Scene : public SceneRegistry {
  public:
	Scene(Opt<Engine::Service> service = {}) noexcept;

	Engine::Service const& engine() const noexcept { return m_engineService; }
	Engine::Executor& executor() const { return m_engineService.executor(); }
	ShaderBufferMap& shaderBufferMap() const;
	graphics::ShaderBuffer& shaderBuffer(Hash id) const;

	virtual void open();
	virtual void tick(Time_s dt);
	virtual void render(graphics::RenderPass&, ShaderSceneView const&) {}
	virtual void close();

  protected:
	Engine::Service m_engineService;
	ShaderBufferMap* m_shaderBufferMap{};

	friend class SceneManager;
};
} // namespace le
