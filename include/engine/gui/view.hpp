#pragma once
#include <engine/gui/tree.hpp>
#include <engine/input/frame.hpp>
#include <engine/utils/owner.hpp>

namespace le::graphics {
class VRAM;
}

namespace le::input {
struct State;
}

namespace le::gui {
class View;
class ViewStack;

enum class Unit { eRelative, eAbsolute };

template <typename T>
struct Measure {
	using type = T;

	T value{};
	T delta{};
	Unit unit{};

	constexpr Measure(T value = {}, T delta = {}, Unit unit = Unit::eRelative) noexcept : value(value), delta(delta), unit(unit) {}
	constexpr Measure(Unit unit) noexcept : Measure({}, {}, unit) {}

	constexpr T operator()(T const& full) noexcept { return unit == Unit::eRelative ? value * full + delta : value; }
};

struct Canvas {
	Measure<glm::vec2> size = {{1.0f, 1.0f}};
	Measure<glm::vec2> centre;
};

class View : public TreeRoot {
  public:
	enum class Block { eNone, eBlock };

	View(not_null<ViewStack*> parent, Block block = {}) noexcept : m_block(block), m_parent(parent) {}

	bool popRecurse(TreeNode const* node) noexcept;

	TreeNode* leafHit(glm::vec2 point) const noexcept;
	void update(input::Frame const& frame, glm::vec2 offset);

	void setDestroyed() noexcept { m_remove = true; }
	bool destroyed() const noexcept { return m_remove; }

	Canvas m_canvas;
	Block m_block;

  private:
	virtual void onUpdate(input::Frame const&) {}

	not_null<ViewStack*> m_parent;
	bool m_remove = false;
};

class ViewStack : public utils::Owner<View> {
  public:
	using Owner::container_t;

	ViewStack(not_null<graphics::VRAM*> vram) noexcept : m_vram(vram) {}

	template <typename T, typename... Args>
		requires(is_derived_v<T>)
	T& push(Args&&... args) { return Owner::template push<T>(this, std::forward<Args>(args)...); }

	void update(input::Frame const& frame, glm::vec2 offset = {});
	View* top() const noexcept { return m_ts.empty() ? nullptr : m_ts.back().get(); }
	container_t const& views() const { return m_ts; }

	not_null<graphics::VRAM*> m_vram;
};
} // namespace le::gui
