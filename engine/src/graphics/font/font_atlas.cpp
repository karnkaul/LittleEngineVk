#include <le/graphics/font/font_atlas.hpp>

namespace le::graphics {
FontAtlas::FontAtlas(NotNull<GlyphSlot::Factory*> slot_factory, CreateInfo create_info) : m_page(slot_factory, create_info.height) {
	struct Entry {
		Codepoint codepoint{};
		Glyph glyph{};
		OffsetRect uv_offset{};
	};

	auto writer = DynamicAtlas::Writer{m_atlas};
	auto bitmap_buffer = std::vector<std::byte>{};
	auto entries = std::vector<Entry>{};

	auto const add_codepoint = [&](Codepoint const codepoint) {
		auto const slot = m_page.slot_for(codepoint);
		if (!slot) { return; }

		auto glyph = Glyph{
			.advance = {slot.advance.x >> 6, slot.advance.y >> 6},
			.extent = slot.pixmap.extent,
			.left_top = slot.left_top,
		};
		auto entry = Entry{.codepoint = codepoint, .glyph = glyph};
		if (slot.has_pixmap()) {
			auto const glyph_bitmap = m_page.make_bitmap(slot.pixmap);
			bitmap_buffer.reserve(glyph_bitmap.bytes.size_bytes() * 4);
			for (auto const byte : glyph_bitmap.bytes) {
				auto const bytes = std::array<std::byte, Bitmap::channels_v>{std::byte{0xff}, std::byte{0xff}, std::byte{0xff}, byte};
				bitmap_buffer.insert(bitmap_buffer.end(), bytes.begin(), bytes.end());
			}
			auto const bitmap = Bitmap{.bytes = bitmap_buffer, .extent = glyph_bitmap.extent};
			entry.uv_offset = writer.enqueue(bitmap);
			bitmap_buffer.clear();
		}

		entries.push_back(entry);
	};

	add_codepoint(Codepoint::eTofu);

	for (auto const [first, last] : create_info.codepoint_ranges) {
		for (auto codepoint = first; codepoint != last; codepoint = static_cast<Codepoint>(static_cast<int>(codepoint) + 1)) { add_codepoint(codepoint); }
	}
	writer.write();

	for (auto& entry : entries) {
		entry.glyph.uv_rect = m_atlas.uv_rect_for(entry.uv_offset);
		m_glyphs.insert_or_assign(entry.codepoint, entry.glyph);
	}
}

auto FontAtlas::glyph_for(Codepoint codepoint) const -> Glyph {
	if (auto const it = m_glyphs.find(codepoint); it != m_glyphs.end()) { return it->second; }
	return {};
}
} // namespace le::graphics
