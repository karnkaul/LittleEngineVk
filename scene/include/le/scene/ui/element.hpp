#pragma once
#include <le/core/radians.hpp>
#include <le/core/time.hpp>
#include <le/graphics/render_object.hpp>
#include <le/scene/ui/rect_transform.hpp>

namespace le::ui {
using graphics::RenderObject;

class View;

class Element {
  public:
	Element() = default;
	Element(Element const&) = default;
	Element(Element&&) = default;
	auto operator=(Element const&) -> Element& = default;
	auto operator=(Element&&) -> Element& = default;

	virtual ~Element() = default;

	virtual auto setup() -> void = 0;
	virtual auto tick(Duration dt) -> void = 0;
	virtual auto render(std::vector<RenderObject>& out) const -> void = 0;

	[[nodiscard]] auto is_destroyed() const -> bool { return m_destroyed; }
	auto set_destroyed() -> void { m_destroyed = true; }

	[[nodiscard]] auto get_parent() const -> View& { return *m_parent_view; }
	[[nodiscard]] auto get_parent_matrix() const -> glm::mat4 const& { return m_parent_mat; }

	[[nodiscard]] auto local_matrix() const -> glm::mat4;

	RectTransform transform{};
	float z_index{};
	Radians rotation{};

  private:
	glm::mat4 m_parent_mat{1.0f};
	Ptr<View> m_parent_view{};
	bool m_destroyed{};

	friend class View;
};
} // namespace le::ui
