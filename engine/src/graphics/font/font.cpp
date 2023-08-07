#include <le/error.hpp>
#include <le/graphics/device.hpp>
#include <le/graphics/font/font.hpp>

namespace le::graphics {
auto Font::try_make(std::vector<std::byte> file_bytes) -> std::optional<Font> {
	auto ret = std::optional<Font>{};
	auto slot_factory = Device::self().get_font_library().load(std::move(file_bytes));
	if (!slot_factory) { return ret; }
	ret.emplace(std::move(slot_factory));
	return ret;
}

Font::Font(NotNull<std::unique_ptr<GlyphSlot::Factory>> slot_factory) : m_slot_factory(std::move(slot_factory)) {}

auto Font::get_font_atlas(TextHeight height) -> FontAtlas& {
	height = clamp(height);
	if (auto it = m_atlases.find(height); it != m_atlases.end()) { return it->second; }

	auto const create_info = FontAtlas::CreateInfo{.height = height};
	auto [it, _] = m_atlases.insert_or_assign(height, FontAtlas{m_slot_factory.get().get(), create_info});
	return it->second;
}

struct Font::Pen::Writer {
	// NOLINTNEXTLINE
	Font::Pen const& pen;

	template <typename Func>
	void operator()(const std::string_view line, Func func) const {
		for (char const ch : line) {
			if (ch == '\n') { return; }
			auto glyph = pen.m_font.glyph_for(pen.m_height, static_cast<Codepoint>(ch));
			if (!glyph) { glyph = pen.m_font.glyph_for(pen.m_height, Codepoint::eTofu); }
			if (!glyph) { continue; }
			func(glyph);
		}
	}
};

auto Font::Pen::advance(std::string_view line) -> Pen& {
	Writer{*this}(line, [this](Glyph const& glyph) { cursor += m_scale * glm::vec3{glyph.advance, 0.0f}; });
	return *this;
}

auto Font::Pen::generate_quads(std::string_view line) -> Geometry {
	auto ret = Geometry{};
	generate_quads(ret, line);
	return ret;
}

auto Font::Pen::generate_quads(Geometry& out, std::string_view line) -> Pen& {
	Writer{*this}(line, [this, &out](Glyph const& glyph) {
		auto const rect = glyph.rect(cursor, m_scale);
		auto const quad = Quad{
			.size = rect.extent(),
			.uv = glyph.uv_rect,
			.rgb = vertex_colour,
			.origin = glm::vec3{rect.centre(), 0.0f},
		};
		out.append(quad);
		cursor += m_scale * glm::vec3{glyph.advance, 0.0f};
	});
	return *this;
}

auto Font::Pen::calc_line_extent(std::string_view line) const -> glm::vec2 {
	auto ret = glm::vec2{};
	Writer{*this}(line, [&ret](Glyph const& glyph) {
		ret.x += glyph.advance.x;
		ret.y = std::max(ret.y, glyph.extent.y);
	});
	return ret;
}
} // namespace le::graphics
