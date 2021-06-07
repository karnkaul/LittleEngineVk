#pragma once
#include <engine/gui/style.hpp>
#include <engine/gui/tree.hpp>
#include <engine/owner.hpp>

namespace le::graphics {
class VRAM;
}

namespace le::input {
struct State;
}

namespace le::gui {
class View;
class ViewStack;

class View : public TreeRoot {
  public:
	enum class Block { eNone, eBlock };

	View(not_null<ViewStack*> parent, Block block = {}) noexcept : m_block(block), m_parent(parent) {}

	TreeNode* leafHit(glm::vec2 point) const noexcept;
	void update(Viewport const& view, glm::vec2 fbSize, glm::vec2 wSize, glm::vec2 offset);

	void setDestroyed() noexcept { m_remove = true; }
	bool destroyed() const noexcept { return m_remove; }

	Block m_block;

  private:
	virtual void onUpdate(input::Space const&) {}

	not_null<ViewStack*> m_parent;
	bool m_remove = false;
};

class ViewStack : public Owner<View> {
  public:
	using Owner::container_t;

	ViewStack(not_null<graphics::VRAM*> vram) noexcept : m_vram(vram) {}

	template <typename T, typename... Args>
		requires(is_derived_v<T>)
	T& push(Args&&... args) { return Owner::template push<T>(this, std::forward<Args>(args)...); }

	void update(input::State const& state, Viewport const& view, glm::vec2 fbSize, glm::vec2 wSize, glm::vec2 offset = {});
	View* top() const noexcept { return m_ts.empty() ? nullptr : m_ts.back().get(); }
	container_t const& views() const { return m_ts; }

	not_null<graphics::VRAM*> m_vram;
};
} // namespace le::gui
