#include <context/device_impl.hpp>
#include <core/utils/expect.hpp>
#include <graphics/font/face.hpp>
#include <unordered_map>

namespace le::graphics {
using SlotMap = std::unordered_map<Codepoint, FontFace::Slot, std::hash<Codepoint::type>>;

namespace {
FontFace::Slot makeSlot(FTFace const face, Codepoint const cp) noexcept {
	EXPECT(face);
	FontFace::Slot ret;
	FTFace::ID const id = cp == Codepoint{} ? 0 : face.glyphIndex(cp);
	if (face.loadGlyph(id)) {
		auto const& slot = *face.face->glyph;
		ret.codepoint = id == 0 ? Codepoint{} : cp;
		ret.advance = {slot.advance.x >> 6, slot.advance.y >> 6};
		ret.pixmap.extent = {slot.bitmap.width, slot.bitmap.rows};
		ret.pixmap.bytes = face.buildGlyphImage();
		ret.pixmap.extent = face.glyphExtent();
		ret.topLeft = {slot.bitmap_left, slot.bitmap_top};
	}
	return ret;
}
} // namespace

struct FontFace::Impl {
	SlotMap map;
	FTUnique<FTFace> face;
	Height height;
};

FontFace::FontFace(not_null<Device*> device) : m_impl(std::make_unique<Impl>()), m_device(device) {}

FontFace::FontFace(FontFace&&) noexcept = default;
FontFace& FontFace::operator=(FontFace&&) noexcept = default;
FontFace::~FontFace() noexcept = default;

bool FontFace::load(Span<std::byte const> ttf, Height height) noexcept {
	m_impl->face = FTFace::make(*m_device->impl().ftLib, ttf);
	if (m_impl->face) {
		m_impl->height = height;
		m_impl->face->setPixelSize({0U, height});
		m_impl->map.emplace(Codepoint{}, makeSlot(*m_impl->face, {}));
		return true;
	}
	return false;
}

FontFace::operator bool() const noexcept { return static_cast<bool>(m_impl->face); }

FontFace::Slot const& FontFace::slot(Codepoint cp) noexcept {
	if (auto it = m_impl->map.find(cp); it != m_impl->map.end()) { return it->second; }
	if (m_impl->face) {
		auto slot = makeSlot(*m_impl->face, cp);
		auto [it, _] = m_impl->map.emplace(cp, slot);
		return it->second;
	}
	static Slot const s_none{};
	return s_none;
}

FontFace::Height FontFace::height() const noexcept { return m_impl->height; }
std::size_t FontFace::slotCount() const noexcept { return m_impl->map.size(); }
void FontFace::clearSlots() noexcept { m_impl->map.clear(); }
} // namespace le::graphics
