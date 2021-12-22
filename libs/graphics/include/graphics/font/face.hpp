#pragma once
#include <core/not_null.hpp>
#include <core/span.hpp>
#include <graphics/bitmap.hpp>
#include <graphics/font/glyph.hpp>
#include <ktl/either.hpp>
#include <memory>

namespace le::graphics {
class Device;

struct CharSize {
	glm::uvec2 size64 = {0U, 16U * 64U};
	glm::uvec2 resolution = {300U, 300U};
};

struct PixelSize {
	glm::uvec2 size = {0U, 16U};
};

class FontFace {
  public:
	using Size = ktl::either<CharSize, PixelSize>;
	struct Slot {
		Bitmap pixmap;
		glm::ivec2 topLeft{};
		glm::ivec2 advance{};
		Codepoint codepoint;

		bool hasBitmap() const noexcept { return pixmap.extent.x > 0 && pixmap.extent.y > 0; }
	};

	FontFace(not_null<Device*> device);
	FontFace(FontFace&&) noexcept;
	FontFace& operator=(FontFace&&) noexcept;
	~FontFace() noexcept;

	bool load(Span<std::byte const> ttf, Size size = CharSize()) noexcept;
	explicit operator bool() const noexcept;

	Slot const& slot(Codepoint cp) const noexcept;

	std::size_t slotCount() const noexcept;
	void clearSlots() noexcept;

  private:
	struct Impl;

	std::unique_ptr<Impl> m_impl;
	not_null<Device*> m_device;
};
} // namespace le::graphics
