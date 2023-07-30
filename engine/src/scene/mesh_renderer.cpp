#include <spaced/core/enumerate.hpp>
#include <spaced/scene/entity.hpp>
#include <spaced/scene/mesh_renderer.hpp>

namespace spaced {
auto MeshRenderer::set_mesh(NotNull<graphics::Mesh const*> mesh) -> void {
	m_mesh = mesh;
	if (m_mesh->skeleton != nullptr) { m_joint_matrices.resize(mesh->skeleton->ordered_joint_ids.size(), glm::identity<glm::mat4>()); }
}

auto MeshRenderer::update_joints(NodeLocator node_locator) -> void {
	if (m_mesh == nullptr || m_mesh->skeleton == nullptr) { return; }
	m_joint_matrices.resize(m_mesh->skeleton->ordered_joint_ids.size(), glm::mat4{1.0f});
	for (auto const [id, index] : enumerate(m_mesh->skeleton->ordered_joint_ids)) {
		m_joint_matrices[index] = node_locator.global_transform(node_locator.get(id)) * m_mesh->skeleton->inverse_bind_matrices[index];
	}
}

auto MeshRenderer::render_to(std::vector<graphics::RenderObject>& out) const -> void {
	if (m_mesh == nullptr) { return; }
	auto const parent = get_entity().get_transform().matrix();
	out.reserve(out.size() + m_mesh->primitives.size());
	for (auto const& primitive : m_mesh->primitives) {
		out.push_back(graphics::RenderObject{
			.material = &graphics::Material::or_default(primitive.material),
			.primitive = primitive.primitive,
			.parent = parent,
			.instances = instances,
			.joints = m_joint_matrices,
			.pipeline_state = pipeline_state,
		});
	}
}
} // namespace spaced
