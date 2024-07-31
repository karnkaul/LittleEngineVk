#include <levk/core/error.hpp>
#include <levk/skeleton.hpp>

namespace levk {
Skeleton::Skeleton(CreateInfo create_info)
	: m_joint_tree(std::move(create_info.joint_tree)), m_inverse_bind_matrices(create_info.inverse_bind_matrices), m_joint_ids(create_info.joint_ids),
	  m_animations(create_info.animations) {
	m_joint_matrices.resize(m_joint_ids.size(), identity_mat_v);
	if (!m_animations.empty()) { animation = m_animations.front(); }
}

void Skeleton::tick(Seconds const dt) {
	if (animation == nullptr) { return; }
	elapsed += dt;
	if (elapsed > animation->get_duration()) { elapsed = {}; }
	animation->update(m_joint_tree, elapsed);
	m_joint_matrices.clear();
	m_joint_matrices.reserve(m_joint_ids.size());
	for (auto const [id, ibm] : std::ranges::zip_view(m_joint_ids, m_inverse_bind_matrices)) {
		auto const* joint = m_joint_tree.get_node(id);
		if (joint == nullptr) { continue; }
		m_joint_matrices.push_back(m_joint_tree.get_model_matrix(*joint) * ibm);
	}
}
} // namespace levk
