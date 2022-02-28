#include <levk/core/utils/expect.hpp>
#include <levk/graphics/render/draw_list.hpp>
#include <levk/graphics/render/pipeline.hpp>

namespace le::graphics {
DrawList& DrawList::push(Span<DrawPrimitive const> primitives, glm::mat4 matrix, std::optional<vk::Rect2D> scissor) {
	std::size_t const start = m_drawPrimitives.size();
	m_drawPrimitives.reserve(start + m_drawPrimitives.size());
	std::copy(primitives.begin(), primitives.end(), PrimitveInserter{&m_drawPrimitives});
	return push(start, matrix, scissor);
}

void DrawList::clear() noexcept {
	m_matrices.clear();
	m_drawPrimitives.clear();
	m_scissors.clear();
	m_entries.clear();
}

DrawList& DrawList::push(std::size_t primitiveStart, glm::mat4 matrix, std::optional<vk::Rect2D> scissor) {
	if (primitiveStart >= m_drawPrimitives.size()) { return *this; }
	Entry entry;
	entry.primitives = {primitiveStart, m_drawPrimitives.size() - primitiveStart};
	entry.matrix = m_matrices.size();
	m_matrices.push_back(matrix);
	if (scissor) {
		entry.scissor = m_scissors.size();
		m_scissors.push_back(*scissor);
	}
	m_entries.push_back(entry);
	m_entryBindings.push_back({});
	return *this;
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

glm::mat4 const& DrawList::iterator::matrix() const { return m_list->m_matrices[m_list->m_entries[m_index].matrix]; }
DescriptorBindings& DrawList::iterator::indices() const { return m_list->m_entryBindings[m_index]; }

Opt<vk::Rect2D const> DrawList::iterator::scissor() const {
	auto const& s = m_list->m_entries[m_index].scissor;
	return s ? &m_list->m_scissors[*s] : nullptr;
}

Span<DrawList::Obj const> DrawList::iterator::prims() const {
	auto const& p = m_list->m_entries[m_index].primitives;
	return Span(&m_list->m_drawPrimitives[p.first], p.second);
}

DrawList::iterator DrawList::begin() const { return {*this, 0U}; }
DrawList::iterator DrawList::end() const { return {*this, m_entries.size()}; }
} // namespace le::graphics
