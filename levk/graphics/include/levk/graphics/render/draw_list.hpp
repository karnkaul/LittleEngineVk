#pragma once
#include <glm/mat4x4.hpp>
#include <levk/core/span.hpp>
#include <levk/graphics/draw_primitive.hpp>
#include <levk/graphics/draw_view.hpp>
#include <levk/graphics/render/descriptor_set.hpp>
#include <levk/graphics/render/pipeline.hpp>
#include <optional>
#include <vector>

namespace le::graphics {
struct DrawBindings {
	mutable std::optional<std::size_t> indices[max_bindings_v]{};

	void bind(u32 set, std::size_t index) const { indices[set] = index; }
};

class DrawList {
  public:
	class iterator;
	using const_iterator = iterator;

	struct Obj {
		DrawBindings bindings{};
		DrawPrimitive primitive{};
	};

	DrawList& push(Span<DrawPrimitive const> primitives, glm::mat4 matrix = glm::mat4(1.0f), std::optional<vk::Rect2D> scissor = {});
	template <DrawPrimitiveAPI T>
	DrawList& add(T const& t, glm::mat4 matrix = glm::mat4(1.0f), std::optional<vk::Rect2D> scissor = {});

	std::size_t size() const noexcept { return m_entries.size(); }
	void clear() noexcept;

	const_iterator begin() const;
	const_iterator end() const;

	DrawBindings m_bindings{};

  private:
	DrawList& push(std::size_t primitiveStart, glm::mat4 matrix, std::optional<vk::Rect2D> scissor);

	template <graphics::DrawPrimitiveAPI T>
	static std::size_t size(T const&) {
		return 0U;
	}

	template <graphics::DrawPrimitiveAPI T>
		requires requires(T t) {
			{ AddDrawPrimitives<T>{}.size(t) } -> std::convertible_to<std::size_t>;
		}
	static std::size_t size(T const& t) { return AddDrawPrimitives<T>{}.size(t); }

	struct Entry {
		std::size_t matrix{};
		std::pair<std::size_t, std::size_t> primitives{};
		std::optional<std::size_t> scissor{};
	};

	struct PrimitveInserter;

	std::vector<glm::mat4> m_matrices{};
	std::vector<Obj> m_drawPrimitives{};
	std::vector<vk::Rect2D> m_scissors{};
	std::vector<Entry> m_entries{};
	std::vector<DrawBindings> m_entryBindings{};
};

struct DrawObject {
	Span<DrawList::Obj const> objs{};
	glm::mat4 const& matrix;
	DrawBindings const& bindings;
	Opt<vk::Rect2D const> scissor{};
};

class DrawList::iterator {
  public:
	using value_type = DrawObject;
	using iterator_category = std::bidirectional_iterator_tag;

	iterator() = default;

	DrawObject operator*() const { return {prims(), matrix(), indices(), scissor()}; }

	iterator& operator++();
	iterator operator++(int);
	iterator& operator--();
	iterator operator--(int);

	bool operator==(iterator const& rhs) const { return m_index == rhs.m_index; }

  private:
	iterator(DrawList const& list, std::size_t index) : m_list(&list), m_index(index) {}

	Span<DrawList::Obj const> prims() const;
	glm::mat4 const& matrix() const;
	DrawBindings const& indices() const;
	Opt<vk::Rect2D const> scissor() const;

	DrawList const* m_list{};
	std::size_t m_index{};

	friend class DrawList;
};

// impl

struct DrawList::PrimitveInserter {
	using value_type = void;
	using difference_type = std::ptrdiff_t;

	std::vector<Obj>* primitives{};

	PrimitveInserter& operator=(DrawPrimitive dp) {
		if (dp) { primitives->push_back({{}, std::move(dp)}); }
		return *this;
	}

	PrimitveInserter& operator*() { return *this; }
	PrimitveInserter& operator++() { return *this; }
	PrimitveInserter& operator++(int) { return *this; }
};

template <graphics::DrawPrimitiveAPI T>
DrawList& DrawList::add(T const& t, glm::mat4 matrix, std::optional<vk::Rect2D> scissor) {
	auto const start = m_drawPrimitives.size();
	m_drawPrimitives.reserve(start + size(t));
	AddDrawPrimitives<T>{}(t, PrimitveInserter{&m_drawPrimitives});
	return push(start, matrix, scissor);
}
} // namespace le::graphics
