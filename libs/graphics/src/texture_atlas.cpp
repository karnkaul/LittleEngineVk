#include <graphics/command_buffer.hpp>
#include <graphics/texture_atlas.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
TextureAtlas::TextureAtlas(not_null<VRAM*> vram, CreateInfo const& info)
	: m_texture(vram, info.sampler, Colour(), {info.maxWidth, info.initialHeight}), m_vram(vram) {}

TextureAtlas::Img TextureAtlas::get(ID id) const noexcept {
	if (auto it = m_data.entries.find(id); it != m_data.entries.end()) { return Img{getUV(it->second), it->second.extent}; }
	return {};
}

bool TextureAtlas::add(ID id, Bitmap const& bitmap, CommandBuffer const& cb) {
	if (!prepAtlas(bitmap.extent, cb)) { return false; }
	utils::copySub(m_vram, cb, bitmap, m_texture.image(), m_data.head);
	Entry entry{bitmap.extent, m_data.head};
	m_data.head.x += bitmap.extent.x;
	m_data.rowHeight = std::max(m_data.rowHeight, bitmap.extent.y);
	m_data.entries.insert_or_assign(id, entry);
	return true;
}

bool TextureAtlas::setUV(ID id, Span<Vertex> quad) const noexcept {
	EXPECT(quad.size() >= 4U);
	if (auto img = get(id); img.extent.x > 0U && quad.size() >= 4U) {
		quad[0].texCoord = {img.uv.topLeft.x, img.uv.bottomRight.y};
		quad[1].texCoord = img.uv.bottomRight;
		quad[2].texCoord = {img.uv.bottomRight.x, img.uv.topLeft.y};
		quad[3].texCoord = img.uv.topLeft;
		return true;
	}
	return false;
}

UVQuad TextureAtlas::getUV(Entry const& entry) const noexcept {
	auto const& itex = m_texture.image().extent2D();
	auto const ftex = glm::vec2(f32(itex.x), f32(itex.y));
	auto const fextent = glm::vec2(f32(entry.extent.x), f32(entry.extent.y));
	auto const foffset = glm::vec2(f32(entry.offset.x), f32(entry.offset.y));
	UVQuad ret;
	ret.topLeft = foffset / ftex;
	ret.bottomRight = (foffset + glm::vec2(fextent.x, fextent.y)) / ftex;
	return ret;
}

bool TextureAtlas::prepAtlas(Extent2D extent, CommandBuffer const& cb) {
	auto const& itex = m_texture.image().extent2D();
	if (extent.x > itex.x) { return false; }
	auto const remain = itex - m_data.head;
	bool resize = false;
	if (extent.y > remain.y) { // y overflow
		resize = true;
	} else if (extent.x > remain.x) {				  // x overflow
		if (extent.y + m_data.rowHeight > remain.y) { // insufficient y
			resize = true;
		} else {
			nextRow();
		}
	}
	bool ret = true;
	if (resize) {
		ret = m_texture.resizeCopy(cb, {itex.x, itex.y * 2U});
		if (ret) { nextRow(); }
	}
	return ret;
}

void TextureAtlas::nextRow() noexcept {
	m_data.head.y += m_data.rowHeight;
	m_data.head.x = m_data.rowHeight = 0;
}
} // namespace le::graphics
