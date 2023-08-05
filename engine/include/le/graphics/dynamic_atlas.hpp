#pragma once
#include <le/core/not_null.hpp>
#include <le/graphics/rect.hpp>
#include <le/graphics/texture.hpp>
#include <le/vfs/uri.hpp>
#include <vector>

namespace le::graphics {
struct DynamicAtlasCreateInfo {
	static constexpr auto extent_v = glm::uvec2{512, 512};
	static constexpr auto padding_v = glm::uvec2{4, 4};

	glm::uvec2 initial_extent{extent_v};
	glm::uvec2 padding{padding_v};
	bool mip_map{true};
};

class DynamicAtlas {
  public:
	using CreateInfo = DynamicAtlasCreateInfo;
	class Writer;

	DynamicAtlas(CreateInfo const& create_info = {});

	[[nodiscard]] auto get_texture() const -> Texture const& { return m_texture; }
	[[nodiscard]] auto uv_rect_for(OffsetRect const& offset) const -> UvRect;

  private:
	Texture m_texture{};
	glm::uvec2 m_padding{};
	glm::uvec2 m_cursor{};
	std::uint32_t m_max_height{};
};

class DynamicAtlas::Writer {
  public:
	Writer(Writer&&) = delete;
	Writer(Writer const&) = delete;
	auto operator=(Writer&&) -> Writer& = delete;
	auto operator=(Writer const&) -> Writer& = delete;

	Writer(DynamicAtlas& out) : m_out(out) {}
	~Writer() { write(); }

	[[nodiscard]] auto enqueue(Bitmap const& bitmap) -> OffsetRect;
	auto write() -> void;

  private:
	struct Write {
		BitmapByteSpan<4> bitmap{};
		glm::uvec2 top_left{};
	};

	// NOLINTNEXTLINE
	DynamicAtlas& m_out;
	std::vector<Write> m_writes{};
	std::vector<std::uint8_t> m_buffer{};
	glm::uvec2 m_new_extent{};
};
} // namespace le::graphics
