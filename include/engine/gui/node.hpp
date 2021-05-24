#pragma once
#include <memory>
#include <optional>
#include <vector>
#include <core/not_null.hpp>
#include <core/span.hpp>
#include <engine/ibase.hpp>
#include <engine/render/flex.hpp>
#include <engine/scene/primitive.hpp>
#include <engine/utils/utils.hpp>
#include <glm/vec2.hpp>
#include <graphics/basis.hpp>
#include <graphics/draw_view.hpp>

namespace le {
struct Viewport;
namespace input {
struct Space;
}
} // namespace le
namespace le::gui {
class Node;

using graphics::DrawScissor;

class Root {
  public:
	template <typename T, typename... Args>
	T& push(Args&&... args);
	bool pop(Node& node) noexcept;

	void update(Viewport const& view, glm::vec2 fbSize, glm::vec2 wSize, glm::vec2 offset = {});
	Node* leafHit(glm::vec2 point) const noexcept;

	glm::vec2 m_size = {};
	glm::vec2 m_origin = {};
	std::vector<std::unique_ptr<Node>> m_nodes;
};

class Node : public Root, public IBase {
  public:
	Node(not_null<Root*> root) noexcept;

	Node& offsetBySize(glm::vec2 size, glm::vec2 coeff = {1.0f, 1.0f}) noexcept;

	void update(input::Space const& space);
	glm::vec3 position() const noexcept;
	glm::mat4 model() const noexcept;
	bool hit(glm::vec2 point) const noexcept;

	virtual View<Primitive> primitives() const noexcept { return {}; }

	DrawScissor m_scissor;
	TFlex<glm::vec2> m_local;
	glm::quat m_orientation = graphics::identity;
	f32 m_zIndex = {};
	bool m_hitTest = false;

	not_null<Root*> m_parent;

  private:
	virtual void onUpdate(input::Space const&) {}

	friend class Root;
};

// impl

template <typename T, typename... Args>
T& Root::push(Args&&... args) {
	static_assert(std::is_base_of_v<Node, T>, "T must derive from Node");
	auto t = std::make_unique<T>(this, std::forward<Args>(args)...);
	T& ret = *t;
	m_nodes.push_back(std::move(t));
	return ret;
}

inline Node::Node(not_null<Root*> root) noexcept : m_parent(root) {}
inline Node& Node::offsetBySize(glm::vec2 size, glm::vec2 coeff) noexcept {
	m_size = size;
	m_local.offset = m_size * 0.5f * coeff;
	return *this;
}
inline glm::vec3 Node::position() const noexcept { return {m_origin, m_zIndex}; }
inline glm::mat4 Node::model() const noexcept {
	static constexpr auto base = glm::mat4(1.0f);
	return glm::translate(base, position()) * glm::toMat4(m_orientation);
}
inline bool Node::hit(glm::vec2 point) const noexcept {
	glm::vec2 const s = {m_size.x * 0.5f, -m_size.y * 0.5f};
	return utils::inAABB(point, m_origin - s, m_origin + s);
}
} // namespace le::gui
