#pragma once
#include <le/graphics/subpass.hpp>
#include <le/scene/scene.hpp>

namespace le {
class SceneRenderer : public graphics::Subpass {
  public:
	explicit SceneRenderer(NotNull<Scene const*> scene);

	[[nodiscard]] auto get_load() const -> Load final;
	auto render(vk::CommandBuffer cmd) -> void override;

	NotNull<Scene const*> scene;

  protected:
	auto render_skybox(graphics::Cubemap const& cubemap) -> void;

	graphics::StaticPrimitive m_skybox_cube{};
	graphics::SkyboxMaterial m_skybox_mat{};
	graphics::PipelineState m_skybox_pipeline{};
	std::vector<graphics::RenderObject> m_objects{};
};
} // namespace le
