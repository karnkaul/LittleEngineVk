#pragma once
#include <dumb_tasks/scheduler.hpp>
#include <levk/engine/engine.hpp>
#include <levk/engine/scene/scene_registry.hpp>

namespace le {
class Scene : public SceneRegistry {
  public:
	Scene(Opt<Engine::Service> service = {}) noexcept;

	Engine::Service const& engine() const noexcept { return m_engineService; }
	dts::scheduler& scheduler() const;

	virtual void open() {}
	virtual void tick(Time_s dt);
	virtual void render(graphics::RenderPass&) {}
	virtual void close();

  protected:
	Engine::Service m_engineService;
	dts::scheduler* m_taskScheduler{};

	friend class SceneManager;
};
} // namespace le
