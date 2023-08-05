#include <le/graphics/command_buffer.hpp>
#include <le/graphics/dynamic_atlas.hpp>
#include <le/graphics/rgba.hpp>
#include <le/resources/bin_data.hpp>
#include <optional>

namespace le::graphics {
namespace {
constexpr auto next_pot(std::uint32_t const in) -> std::uint32_t {
	auto ret = std::uint32_t{1};
	while (ret < in) { ret <<= 1; }
	return ret;
}

constexpr auto make_new_extent(glm::uvec2 existing, std::optional<std::uint32_t> x, std::optional<std::uint32_t> y) -> glm::uvec2 {
	if (x) { existing.x = *x; }
	if (y) { existing.y = *y; }
	return existing;
}

auto make_texture(DynamicAtlas::CreateInfo const& create_info) -> Texture {
	auto bytes = std::vector<std::uint8_t>(create_info.initial_extent.x * create_info.initial_extent.y * 4);
	auto sampler = TextureSampler{};
	sampler.wrap_s = sampler.wrap_t = TextureSampler::Wrap::eClampBorder;
	sampler.min = sampler.mag = TextureSampler::Filter::eNearest;
	sampler.border = TextureSampler::Border::eTransparentBlack;
	auto ret = Texture{ColourSpace::eSrgb, create_info.mip_map};
	auto const bitmap = Bitmap{.bytes = bytes, .extent = create_info.initial_extent};
	ret.write(bitmap);
	return ret;
}
} // namespace

DynamicAtlas::DynamicAtlas(CreateInfo const& create_info)
	: m_texture(make_texture(create_info)), m_padding(create_info.padding), m_cursor(create_info.padding) {}

auto DynamicAtlas::uv_rect_for(OffsetRect const& offset) const -> UvRect {
	glm::vec2 const image_extent = glm::uvec2{m_texture.image().extent().width, m_texture.image().extent().height};
	glm::vec2 const top_left = offset.top_left();
	if (top_left.x > image_extent.x || top_left.y > image_extent.y) { return {}; }
	glm::vec2 const cell_extent = offset.bottom_right() - offset.top_left();
	return UvRect{.lt = top_left / image_extent, .rb = (top_left + cell_extent) / image_extent};
}

auto DynamicAtlas::Writer::enqueue(Bitmap const& bitmap) -> OffsetRect {
	auto const texture_extent = glm::uvec2{m_out.m_texture.image().extent().width, m_out.m_texture.image().extent().height};
	auto const current_extent = m_new_extent.x == 0 ? texture_extent : m_new_extent;
	auto const required_extent = bitmap.extent + m_out.m_padding;
	if (m_out.m_cursor.x + required_extent.x > texture_extent.x) {
		m_out.m_cursor.y += m_out.m_max_height + m_out.m_padding.y; // line break
		m_out.m_cursor.x = m_out.m_padding.x;						// carriage return
		m_out.m_max_height = {};
	}
	auto new_width = std::optional<std::uint32_t>{};
	auto new_height = std::optional<std::uint32_t>{};
	if (m_out.m_cursor.x + required_extent.x > current_extent.x) { new_width = next_pot(bitmap.extent.x); }
	if (m_out.m_cursor.y + required_extent.y > current_extent.y) { new_height = next_pot(current_extent.y + required_extent.y); }
	if (new_width || new_height) { m_new_extent = make_new_extent(current_extent, new_width, new_height); }
	auto const ret = OffsetRect{.lt = m_out.m_cursor, .rb = m_out.m_cursor + bitmap.extent};
	m_out.m_cursor.x += required_extent.x;
	m_out.m_max_height = std::max(m_out.m_max_height, required_extent.y);
	auto const offset_bytes = resize_and_overwrite(m_buffer, bitmap.bytes);
	auto const byte_span_bitmap = BitmapByteSpan<4>{.bytes = offset_bytes, .extent = bitmap.extent};
	m_writes.push_back(Write{byte_span_bitmap, ret.top_left()});
	return ret;
}

auto DynamicAtlas::Writer::write() -> void {
	if (m_writes.empty()) { return; }
	if (m_new_extent.x > 0) { m_out.m_texture.m_image.get()->resize({m_new_extent.x, m_new_extent.y}); }
	auto writes = std::vector<ImageWrite>{};
	writes.reserve(m_writes.size());
	for (auto const& in : m_writes) {
		auto out = ImageWrite{
			.bitmap = Bitmap{.bytes = in.bitmap.bytes.make_span(m_buffer), .extent = in.bitmap.extent},
			.top_left = in.top_left,
		};
		writes.push_back(out);
	}
	m_out.m_texture.m_image.get()->overwrite(writes);
	m_writes.clear();
}
} // namespace le::graphics
