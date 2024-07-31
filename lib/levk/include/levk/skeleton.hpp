#pragma once
#include <levk/node_tree.hpp>
#include <levk/tree_animation.hpp>
#include <span>
#include <vector>

namespace levk {
/// \brief Skeletal TreeAnimation.
class SkeletalAnimation : public TreeAnimation<TreeNode> {};

struct SkeletonCreateInfo {
	NodeTree joint_tree{};
	std::span<glm::mat4 const> inverse_bind_matrices{};
	std::span<TreeNodeId const> joint_ids{};
	std::span<NotNull<SkeletalAnimation const*>> animations{};
};

/// \brief Skeleton for one or more skinned meshes.
/// References NodeTree, inverse bind matrices, joints, and animations.
/// Builds joint matrices.
class Skeleton {
  public:
	using CreateInfo = SkeletonCreateInfo;
	using Animation = SkeletalAnimation;

	Skeleton() = default;

	explicit Skeleton(CreateInfo create_info);

	[[nodiscard]] auto get_joint_tree() const -> NodeTree const& { return m_joint_tree; }
	[[nodiscard]] auto get_joint_ids() const -> std::span<TreeNodeId const> { return m_joint_ids; }

	[[nodiscard]] auto get_inverse_bind_matrices() const -> std::span<glm::mat4 const> { return m_inverse_bind_matrices; }
	[[nodiscard]] auto get_joint_matrices() const -> std::span<glm::mat4 const> { return m_joint_matrices; }

	[[nodiscard]] auto get_animations() const -> std::span<NotNull<Animation const*>> { return m_animations; }

	/// \brief Tick current animation and build joint matrices.
	void tick(Seconds dt);

	/// \brief Current animation.
	Ptr<Animation const> animation{};

	/// \brief Animation clock.
	Seconds elapsed{};

	std::string_view name{};

  private:
	NodeTree m_joint_tree{};
	std::span<glm::mat4 const> m_inverse_bind_matrices{};
	std::span<TreeNodeId const> m_joint_ids{};
	std::span<NotNull<Animation const*>> m_animations{};

	std::vector<glm::mat4> m_joint_matrices{};
};
} // namespace levk
