#pragma once
#include <le/graphics/mesh.hpp>
#include <le/scene/render_component.hpp>

namespace le {
class MeshRenderer : public RenderComponent {
  public:
	using RenderComponent::RenderComponent;

	std::vector<graphics::RenderInstance> instances{};
	graphics::PipelineState pipeline_state{};

	auto tick(Duration /*dt*/) -> void override {}
	auto render_to(std::vector<graphics::RenderObject>& out) const -> void override;

	auto set_mesh(NotNull<graphics::Mesh const*> mesh) -> void;
	auto update_joints(NodeLocator node_locator) -> void;

  private:
	Ptr<graphics::Mesh const> m_mesh{};
	std::vector<glm::mat4> m_joint_matrices{};
};
} // namespace le
