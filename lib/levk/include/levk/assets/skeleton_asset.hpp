#pragma once
#include <levk/asset.hpp>
#include <levk/logger.hpp>
#include <levk/skeleton.hpp>

namespace dj {
class Json;
}

namespace levk {
class SkeletonAsset : public IAsset {
  public:
	static constexpr std::string_view type_name_v{"SkeletonAsset"};

	[[nodiscard]] auto get_skeleton() const -> Skeleton const& { return m_skeleton; }

  private:
	[[nodiscard]] auto get_type_name() const -> std::string_view final { return type_name_v; }
	auto load(IAssetStore& store, LoadInfo const& info) -> bool final;

	Logger m_log{std::string{type_name_v}};

	std::string m_name{};
	NodeTree m_joint_tree{};
	std::vector<glm::mat4> m_inverse_bind_matrices{};
	std::vector<TreeNodeId> m_joint_ids{};
	TreeNodeId m_root_joint{TreeNodeId::eNone};
	std::vector<NotNull<SkeletalAnimation const*>> m_animations{};

	Skeleton m_skeleton{};
};
} // namespace levk
