#pragma once
#include <le/scene/scene_renderer.hpp>
#include <le/scene/scene_switcher.hpp>

namespace le {
class SceneManager {
  public:
	auto tick(Duration dt) -> void;
	auto render() const -> void;

	[[nodiscard]] auto get_active_scene() const -> Scene& { return m_switcher.get_active_scene(); }

	auto set_renderer(std::unique_ptr<SceneRenderer> renderer) -> void;

  private:
	SceneSwitcher m_switcher{};
	std::unique_ptr<SceneRenderer> m_renderer{std::make_unique<SceneRenderer>()};
};
} // namespace le
