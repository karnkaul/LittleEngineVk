#pragma once
#include <le/graphics/font/glyph_slot.hpp>
#include <memory>

namespace le::graphics {
class FontLibrary {
  public:
	struct Null;

	FontLibrary(FontLibrary const&) = delete;
	FontLibrary(FontLibrary&&) = delete;
	auto operator=(FontLibrary const&) -> FontLibrary& = delete;
	auto operator=(FontLibrary&&) -> FontLibrary& = delete;

	FontLibrary() = default;
	virtual ~FontLibrary() = default;

	[[nodiscard]] virtual auto load(std::vector<std::byte> bytes) const -> std::unique_ptr<GlyphSlot::Factory> = 0;

	[[nodiscard]] static auto make() -> std::unique_ptr<FontLibrary>;
};

struct FontLibrary::Null : FontLibrary {
	[[nodiscard]] auto load(std::vector<std::byte> /*bytes*/) const -> std::unique_ptr<GlyphSlot::Factory> final {
		return std::make_unique<GlyphSlot::Factory::Null>();
	}
};
} // namespace le::graphics
