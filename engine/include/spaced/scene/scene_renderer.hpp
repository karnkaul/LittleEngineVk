#pragma once
#include <spaced/graphics/render_pass.hpp>
#include <spaced/scene/scene.hpp>

namespace spaced {
class SceneRenderer : public graphics::RenderPass {
  public:
	explicit SceneRenderer(NotNull<Scene const*> scene, graphics::Rgba clear = graphics::black_v);

	auto render(vk::CommandBuffer cmd) -> void override;

	NotNull<Scene const*> scene;

  protected:
	std::vector<graphics::RenderObject> m_objects{};
};
} // namespace spaced
