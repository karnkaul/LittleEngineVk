#pragma once
#include <le/core/not_null.hpp>
#include <le/graphics/font/glyph_slot.hpp>
#include <unordered_map>

namespace le::graphics {
using GlyphBitmap = BitmapView<1>;

class GlyphPage {
  public:
	explicit GlyphPage(NotNull<GlyphSlot::Factory*> slot_factory, TextHeight height = TextHeight::eDefault);

	[[nodiscard]] auto make_bitmap(GlyphSlot::Pixmap const& pixmap) const -> GlyphBitmap;
	[[nodiscard]] auto slot_for(Codepoint codepoint) -> GlyphSlot;

	[[nodiscard]] auto get_slot_factory() const -> GlyphSlot::Factory& { return *m_slot_factory; }
	[[nodiscard]] auto get_text_height() const -> TextHeight { return m_height; }
	[[nodiscard]] auto get_slot_count() const -> std::size_t { return m_slots.size(); }

  private:
	std::vector<std::byte> m_pixmap_buffer{};
	std::unordered_map<Codepoint, GlyphSlot> m_slots{};
	NotNull<GlyphSlot::Factory*> m_slot_factory;
	TextHeight m_height{};
};
} // namespace le::graphics
