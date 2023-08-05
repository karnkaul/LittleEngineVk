#pragma once
#include <le/graphics/mesh.hpp>
#include <le/scene/component.hpp>
#include <optional>

namespace le {
class MeshAnimator : public Component {
  public:
	auto tick(Duration dt) -> void override;

	auto set_skeleton(NotNull<graphics::Skeleton const*> skeleton, std::optional<Id<graphics::Animation>> id = 0) -> void;

	[[nodiscard]] auto get_animations() const -> std::span<Ptr<graphics::Animation const> const>;
	auto set_animation_id(Id<graphics::Animation> index) -> bool;

	[[nodiscard]] auto get_animation_id() const -> std::optional<Id<graphics::Animation>> { return m_active; }
	[[nodiscard]] auto get_animation() const -> Ptr<graphics::Animation const>;

	Duration elapsed{};

  protected:
	Ptr<graphics::Skeleton const> m_skeleton{};
	std::optional<Id<graphics::Animation>> m_active{};
	NodeTree m_joint_tree{};
};
} // namespace le
