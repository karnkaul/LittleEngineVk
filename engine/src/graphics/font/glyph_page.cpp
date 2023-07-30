#include <spaced/graphics/font/glyph_page.hpp>

namespace spaced::graphics {
GlyphPage::GlyphPage(NotNull<GlyphSlot::Factory*> slot_factory, TextHeight height) : m_slot_factory(slot_factory), m_height(height) {}

auto GlyphPage::make_bitmap(GlyphSlot::Pixmap const& pixmap) const -> GlyphBitmap {
	auto ret = GlyphBitmap{.extent = pixmap.extent};
	if (pixmap.bytes.is_empty()) { return ret; }
	ret.bytes = pixmap.bytes.make_span(m_pixmap_buffer);
	return ret;
}

auto GlyphPage::slot_for(Codepoint const codepoint) -> GlyphSlot {
	auto const it = m_slots.find(codepoint);
	if (it != m_slots.end()) { return it->second; }

	m_slot_factory->set_height(m_height);
	auto ret = m_slot_factory->slot_for(codepoint, m_pixmap_buffer);
	if (!ret) { return {}; }

	return m_slots.insert_or_assign(codepoint, ret).first->second;
}
} // namespace spaced::graphics
