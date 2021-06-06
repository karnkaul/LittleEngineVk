#pragma once
#include <engine/gui/tree.hpp>
#include <engine/owner.hpp>

namespace le::gui {
class View;
class ViewStack;

class View : public TreeRoot {
  public:
	View(not_null<ViewStack*> parent) noexcept : m_parent(parent) {}

	TreeNode* leafHit(glm::vec2 point) const noexcept;
	void update(Viewport const& view, glm::vec2 fbSize, glm::vec2 wSize, glm::vec2 offset);

	void setDestroyed() noexcept { m_remove = true; }
	bool destroyed() const noexcept { return m_remove; }

  private:
	not_null<ViewStack*> m_parent;
	bool m_remove = false;
};

class ViewStack : public Owner<View> {
  public:
	using Owner::container_t;

	template <typename T, typename... Args>
	requires requires(T) { derived_v<T>; }
	T& push(Args&&... args) { return Owner::template push<T>(this, std::forward<Args>(args)...); }

	void update(Viewport const& view, glm::vec2 fbSize, glm::vec2 wSize, glm::vec2 offset = {});
	template <typename T = View>
	T* top() const noexcept;

	container_t const& views() const { return m_ts; }
};

// impl

template <typename T>
T* ViewStack::top() const noexcept {
	if (m_ts.empty()) { return nullptr; }
	if constexpr (std::is_same_v<T, View>) {
		return m_ts.back().get();
	} else {
		return dynamic_cast<T*>(m_ts.back().get());
	}
}
} // namespace le::gui
