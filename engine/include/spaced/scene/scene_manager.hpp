#pragma once
#include <spaced/scene/scene_renderer.hpp>
#include <spaced/scene/scene_switcher.hpp>

namespace spaced {
class SceneManager {
  public:
	auto tick(Duration dt) -> void;
	auto render() const -> void;
	auto render(std::span<Ptr<graphics::RenderPass> const> subpasses) const -> void;

	auto set_clear_colour(graphics::Rgba clear) -> void;

	[[nodiscard]] auto get_active_scene() const -> Scene& { return m_switcher.get_active_scene(); }

  private:
	SceneSwitcher m_switcher{};
	std::unique_ptr<SceneRenderer> m_renderer{std::make_unique<SceneRenderer>(m_switcher.m_active.get())};
};
} // namespace spaced
