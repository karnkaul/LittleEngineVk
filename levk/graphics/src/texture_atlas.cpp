#include <levk/graphics/command_buffer.hpp>
#include <levk/graphics/texture_atlas.hpp>
#include <levk/graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
vk::SamplerCreateInfo samplerInfo() {
	auto ret = Sampler::info({vk::Filter::eLinear, vk::Filter::eLinear});
	ret.mipmapMode = vk::SamplerMipmapMode::eLinear;
	ret.addressModeU = ret.addressModeV = ret.addressModeW = vk::SamplerAddressMode::eClampToBorder;
	ret.borderColor = vk::BorderColor::eIntOpaqueBlack;
	return ret;
}
} // namespace

TextureAtlas::TextureAtlas(not_null<VRAM*> vram, CreateInfo const& info)
	: m_sampler(vram->m_device, samplerInfo()),
	  m_texture(vram, m_sampler.sampler(), Colour(), {info.maxWidth, info.initialHeight}, Texture::Payload::eColour, info.mipMaps), m_pad(info.pad),
	  m_vram(vram) {
	m_data.head += m_pad;
}

QuadTex TextureAtlas::get(ID id) const noexcept {
	if (auto it = m_data.entries.find(id); it != m_data.entries.end()) { return QuadTex{getUV(it->second), it->second.extent}; }
	return {};
}

TextureAtlas::Result TextureAtlas::add(ID id, Bitmap const& bitmap, CommandBuffer const& cb) {
	Result ret;
	auto res = prepAtlas(bitmap.extent, cb, ret);
	if (res != Outcome::eOk) { return ret; }
	ret.scratch.buffer = utils::copySub(m_vram, cb, bitmap, m_texture.image(), m_data.head);
	Entry entry{bitmap.extent, m_data.head};
	m_data.head.x += bitmap.extent.x + m_pad.x;
	m_data.rowHeight = std::max(m_data.rowHeight, bitmap.extent.y);
	m_data.entries.insert_or_assign(id, entry);
	return ret;
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

void TextureAtlas::clear() {
	if (!m_data.entries.empty()) {
		m_data = {};
		auto const extent = m_texture.image().extent2D();
		auto const mips = m_texture.image().mipCount() > 1U;
		m_texture = Texture(m_vram, m_sampler.sampler(), Colour(), extent, Texture::Payload::eColour, mips);
	}
}

QuadUV TextureAtlas::getUV(Entry const& entry) const noexcept {
	auto const& itex = m_texture.image().extent2D();
	auto const ftex = glm::vec2(f32(itex.x), f32(itex.y));
	auto const fextent = glm::vec2(f32(entry.extent.x), f32(entry.extent.y));
	auto const foffset = glm::vec2(f32(entry.offset.x), f32(entry.offset.y));
	QuadUV ret;
	ret.topLeft = foffset / ftex;
	ret.bottomRight = (foffset + glm::vec2(fextent.x, fextent.y)) / ftex;
	return ret;
}

TextureAtlas::Outcome TextureAtlas::prepAtlas(Extent2D extent, CommandBuffer const& cb, Result& out) {
	if (extent.x == 0 || extent.y == 0) { return Outcome::eInvalidSize; }
	auto const& itex = m_texture.image().extent2D();
	if (extent.x > itex.x) { return Outcome::eOverflowX; }
	auto const remain = itex - m_data.head;
	bool resize = false;
	if (extent.y + m_pad.y > remain.y) { // y overflow
		resize = true;
	} else if (extent.x + m_pad.x > remain.x) {					// x overflow
		if (extent.y + m_pad.y + m_data.rowHeight > remain.y) { // insufficient y
			resize = true;
		} else {
			nextRow();
		}
	}
	if (resize) {
		if (m_locked) { return Outcome::eSizeLocked; }
		auto res = m_texture.resizeCopy(cb, {itex.x, itex.y * 2U});
		if (!res.outcome) { return Outcome::eResizeFail; }
		out.scratch = std::move(res.scratch);
		nextRow();
	}
	m_texture.wait();
	EXPECT(m_data.head.x + extent.x + m_pad.x <= m_texture.image().extent2D().x);
	EXPECT(m_data.head.y + extent.y + m_pad.y <= m_texture.image().extent2D().y);
	return Outcome::eOk;
}

void TextureAtlas::nextRow() noexcept {
	m_data.head.y += m_data.rowHeight;
	m_data.head.x = 0;
	m_data.head += m_pad;
	m_data.rowHeight = 0U;
}
} // namespace le::graphics
