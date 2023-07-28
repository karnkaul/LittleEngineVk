#pragma once
#include <spaced/graphics/mesh.hpp>
#include <spaced/scene/render_component.hpp>

namespace spaced {
class MeshRenderer : public RenderComponent {
  public:
	std::vector<graphics::RenderInstance> instances{};
	graphics::PipelineState pipeline_state{};

	auto tick([[maybe_unused]] Duration dt) -> void override {}
	auto render_to(std::vector<graphics::RenderObject>& out) const -> void override;

	auto set_mesh(NotNull<graphics::Mesh const*> mesh) -> void;
	auto update_joints(NodeLocator node_locator) -> void;

  private:
	Ptr<graphics::Mesh const> m_mesh{};
	std::vector<glm::mat4> m_joint_matrices{};
};
} // namespace spaced
