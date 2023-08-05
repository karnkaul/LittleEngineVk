#pragma once
#include <le/scene/scene.hpp>
#include <memory>

namespace le {
class SceneSwitcher : public MonoInstance<SceneSwitcher> {
  public:
	auto switch_scene(std::unique_ptr<Scene> next_scene) { m_standby = std::move(next_scene); }

	[[nodiscard]] auto get_active_scene() const -> Scene& { return *m_active; }

  private:
	std::unique_ptr<Scene> m_active{std::make_unique<Scene>()};
	std::unique_ptr<Scene> m_standby{};

	friend class SceneManager;
};
} // namespace le
