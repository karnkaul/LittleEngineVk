#include <levk/core/utils/expect.hpp>
#include <levk/graphics/render/draw_list.hpp>

namespace le::graphics {
DrawList& DrawList::push(Span<DrawPrimitive const> primitives, glm::mat4 matrix, std::optional<vk::Rect2D> scissor) {
	std::size_t const start = this->m_primitives.size();
	m_primitives.reserve(m_primitives.size() + primitives.size());
	for (auto const& primitive : primitives) {
		if (primitive) { m_primitives.push_back(primitive); }
	}
	return push(start, matrix, scissor);
}

DrawList& DrawList::push(std::size_t primitiveStart, glm::mat4 matrix, std::optional<vk::Rect2D> scissor) {
	if (primitiveStart >= m_primitives.size()) { return *this; }
	Entry entry;
	entry.primitives = {primitiveStart, m_primitives.size() - primitiveStart};
	entry.matrix = m_matrices.size();
	m_matrices.push_back(matrix);
	if (scissor) {
		entry.scissor = m_scissors.size();
		m_scissors.push_back(*scissor);
	}
	m_entries.push_back(entry);
	return *this;
}

Span<DrawPrimitive const> DrawList::iterator::prims() const {
	auto const& p = m_list->m_entries[m_index].primitives;
	return Span(&m_list->m_primitives[p.first], p.second);
}

DrawList::iterator& DrawList::iterator::operator++() {
	++m_index;
	return *this;
}

DrawList::iterator DrawList::iterator::operator++(int) {
	auto ret = *this;
	++(*this);
	return ret;
}

DrawList::iterator& DrawList::iterator::operator--() {
	--m_index;
	return *this;
}

DrawList::iterator DrawList::iterator::operator--(int) {
	auto ret = *this;
	--(*this);
	return ret;
}

not_null<glm::mat4 const*> DrawList::iterator::matrix() const { return &m_list->m_matrices[m_list->m_entries[m_index].matrix]; }

Opt<vk::Rect2D const> DrawList::iterator::scissor() const {
	auto const& s = m_list->m_entries[m_index].scissor;
	return s ? &m_list->m_scissors[*s] : nullptr;
}

DrawList::iterator DrawList::begin() const { return {*this, 0U}; }
DrawList::iterator DrawList::end() const { return {*this, m_entries.size()}; }
} // namespace le::graphics
