#pragma once
#include <le/graphics/bitmap.hpp>
#include <le/graphics/font/codepoint.hpp>
#include <le/graphics/font/text_height.hpp>
#include <vector>

namespace le::graphics {
struct GlyphSlot {
	using Pixmap = BitmapByteSpan<1>;

	class Factory;

	Pixmap pixmap{};
	glm::ivec2 left_top{};
	glm::ivec2 advance{};
	Codepoint codepoint{};

	[[nodiscard]] constexpr auto has_pixmap() const -> bool { return !pixmap.bytes.is_empty(); }

	explicit constexpr operator bool() const { return advance.x > 0 || advance.y > 0; }
};

class GlyphSlot::Factory {
  public:
	struct Null;

	Factory(Factory const&) = delete;
	Factory(Factory&&) = delete;
	auto operator=(Factory const&) -> Factory& = delete;
	auto operator=(Factory&&) -> Factory& = delete;

	Factory() = default;
	virtual ~Factory() = default;

	virtual auto set_height(TextHeight height) -> bool = 0;
	[[nodiscard]] virtual auto height() const -> TextHeight = 0;
	[[nodiscard]] virtual auto slot_for(Codepoint codepoint, std::vector<std::uint8_t>& out_bytes) const -> GlyphSlot = 0;

	auto slot_for(Codepoint codepoint, TextHeight height, std::vector<std::uint8_t>& out_bytes) -> GlyphSlot {
		if (!set_height(height)) { return {}; }
		return slot_for(codepoint, out_bytes);
	}
};

struct GlyphSlot::Factory::Null : GlyphSlot::Factory {
	using Factory::Factory;

	auto set_height(TextHeight /*height*/) -> bool final { return true; }
	[[nodiscard]] auto height() const -> TextHeight final { return {}; }
	[[nodiscard]] auto slot_for(Codepoint /*codepoint*/, std::vector<std::uint8_t>& /*out_bytes*/) const -> GlyphSlot final { return {}; }
};
} // namespace le::graphics
