#include <le/scene/mesh_animator.hpp>
#include <le/scene/mesh_renderer.hpp>
#include <le/scene/scene.hpp>

namespace le {
auto MeshAnimator::tick(Duration dt) -> void {
	if (m_skeleton == nullptr) { return; }

	if (!m_active) { return; }

	auto const& animation = m_skeleton->animations[*m_active];
	elapsed += dt;
	animation->update(m_joint_tree, elapsed);
	if (elapsed >= animation->duration()) { elapsed = {}; }

	auto* mesh_renderer = get_entity().find_component<MeshRenderer>();
	if (mesh_renderer == nullptr) { return; }

	mesh_renderer->update_joints(m_joint_tree);
}

auto MeshAnimator::set_skeleton(NotNull<graphics::Skeleton const*> skeleton, std::optional<Id<graphics::Animation>> id) -> void {
	m_skeleton = skeleton;
	m_joint_tree = skeleton->joint_tree;
	if (!id || *id < m_skeleton->animations.size()) { m_active = id; }
}

auto MeshAnimator::get_animations() const -> std::span<Ptr<graphics::Animation const> const> {
	if (m_skeleton == nullptr) { return {}; }
	return m_skeleton->animations;
}

auto MeshAnimator::set_animation_id(Id<graphics::Animation> index) -> bool {
	if (m_skeleton == nullptr || index >= m_skeleton->animations.size()) { return false; }
	m_active = index;
	elapsed = {};
	return true;
}

auto MeshAnimator::get_animation() const -> Ptr<graphics::Animation const> {
	if (!m_active || m_skeleton == nullptr) { return {}; }
	return m_skeleton->animations[*m_active];
}
} // namespace le
