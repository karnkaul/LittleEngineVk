#pragma once
#include <optional>
#include <core/not_null.hpp>
#include <core/span.hpp>
#include <core/utils/vbase.hpp>
#include <engine/gui/rect.hpp>
#include <engine/render/flex.hpp>
#include <engine/scene/primitive.hpp>
#include <engine/utils/owner.hpp>
#include <graphics/basis.hpp>
#include <graphics/draw_view.hpp>

namespace le {
struct Viewport;
namespace input {
struct Space;
}
} // namespace le

namespace le::gui {
class TreeNode;

using graphics::DrawScissor;

DrawScissor scissor(input::Space const& space, glm::vec2 centre = {}, glm::vec2 halfSize = {0.5f, -0.5f}, bool normalised = true) noexcept;

class TreeRoot : public utils::VBase, public utils::Owner<TreeNode> {
  public:
	using Owner::container_t;

	template <typename T, typename... Args>
		requires(is_derived_v<T>)
	T& push(Args&&... args) { return Owner::template push<T>(this, std::forward<Args>(args)...); }

	container_t const& nodes() const noexcept { return m_ts; }

	Rect m_rect;
};

class TreeNode : public TreeRoot {
  public:
	TreeNode(not_null<TreeRoot*> root) noexcept : m_parent(root) {}

	TreeNode& offset(glm::vec2 size, glm::vec2 coeff = {1.0f, 1.0f}) noexcept;
	void update(input::Space const& space);

	glm::vec3 position() const noexcept { return m_rect.position(m_zIndex); }
	glm::mat4 model() const noexcept;
	bool hit(glm::vec2 point) const noexcept { return m_hitTest && m_rect.hit(point); }

	virtual Span<Primitive const> primitives() const noexcept { return {}; }

	DrawScissor m_scissor;
	glm::quat m_orientation = graphics::identity;
	f32 m_zIndex = {};
	bool m_hitTest = false;

	not_null<TreeRoot*> m_parent;

  private:
	virtual void onUpdate(input::Space const&) {}
};

// impl

inline TreeNode& TreeNode::offset(glm::vec2 size, glm::vec2 coeff) noexcept {
	m_rect.offset(size, coeff);
	return *this;
}
inline glm::mat4 TreeNode::model() const noexcept {
	static constexpr auto base = glm::mat4(1.0f);
	return glm::translate(base, position()) * glm::toMat4(m_orientation);
}
} // namespace le::gui
