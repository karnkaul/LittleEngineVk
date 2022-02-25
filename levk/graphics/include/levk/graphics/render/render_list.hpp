#pragma once
#include <glm/mat4x4.hpp>
#include <levk/core/span.hpp>
#include <levk/graphics/draw_primitive.hpp>
#include <levk/graphics/draw_view.hpp>
#include <levk/graphics/render/pipeline.hpp>
#include <optional>
#include <vector>

namespace le::graphics {
class DrawList {
  public:
	class iterator;
	using const_iterator = iterator;

	DrawList& push(Span<DrawPrimitive const> primitives, glm::mat4 matrix = glm::mat4(1.0f), std::optional<vk::Rect2D> scissor = {});
	template <typename T>
	DrawList& add(T const& t, glm::mat4 matrix = glm::mat4(1.0f), std::optional<vk::Rect2D> scissor = {}) {
		auto const start = m_primitives.size();
		DrawPrimitiveAdder<T>{}(t, std::back_inserter(m_primitives));
		return push(start, matrix, scissor);
	}

	iterator begin() const;
	iterator end() const;

  private:
	DrawList& push(std::size_t primitiveStart, glm::mat4 matrix, std::optional<vk::Rect2D> scissor);

	struct Entry {
		std::size_t matrix{};
		std::pair<std::size_t, std::size_t> primitives{};
		std::optional<std::size_t> scissor{};
	};

	std::vector<glm::mat4> m_matrices;
	std::vector<DrawPrimitive> m_primitives;
	std::vector<vk::Rect2D> m_scissors;
	std::vector<Entry> m_entries;
};

struct DrawObject {
	not_null<glm::mat4 const*> matrix;
	Span<DrawPrimitive const> primitives;
	Opt<vk::Rect2D const> scissor{};
};

class DrawList::iterator {
  public:
	using value_type = DrawObject;
	using iterator_category = std::bidirectional_iterator_tag;

	iterator() = default;

	DrawObject operator*() const { return {matrix(), prims(), scissor()}; }

	iterator& operator++();
	iterator operator++(int);
	iterator& operator--();
	iterator operator--(int);

	bool operator==(iterator const& rhs) const { return m_index == rhs.m_index; }

  private:
	iterator(DrawList const& list, std::size_t index) : m_list(&list), m_index(index) {}

	Span<DrawPrimitive const> prims() const;
	not_null<glm::mat4 const*> matrix() const;
	Opt<vk::Rect2D const> scissor() const;

	DrawList const* m_list{};
	std::size_t m_index{};

	friend class DrawList;
};

enum struct RenderOrder : s64 { eDefault };

} // namespace le::graphics
