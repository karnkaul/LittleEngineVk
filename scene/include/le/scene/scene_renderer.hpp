#pragma once
#include <le/graphics/render_frame.hpp>
#include <le/scene/scene.hpp>

namespace le {
class SceneRenderer {
  public:
	SceneRenderer(SceneRenderer const&) = delete;
	SceneRenderer(SceneRenderer&&) = delete;
	auto operator=(SceneRenderer const&) -> SceneRenderer& = delete;
	auto operator=(SceneRenderer&&) -> SceneRenderer& = delete;

	virtual ~SceneRenderer() = default;

	explicit SceneRenderer();

	auto render(Scene const& scene) -> graphics::RenderFrame const&;

  protected:
	auto render_skybox(graphics::Cubemap const& cubemap) -> void;

	graphics::StaticPrimitive m_skybox_cube{};
	graphics::SkyboxMaterial m_skybox_mat{};
	graphics::PipelineState m_skybox_pipeline{};
	std::vector<graphics::RenderObject> m_objects{};
	std::optional<graphics::RenderFrame> m_render_frame{};
};
} // namespace le
