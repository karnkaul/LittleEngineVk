#pragma once
#include <spaced/graphics/bitmap.hpp>
#include <memory>

namespace spaced::graphics {
class ImageFile {
  public:
	auto decompress(std::span<std::uint8_t const> compressed, int channels = 4) -> bool;

	[[nodiscard]] auto bitmap() const -> Bitmap;

	[[nodiscard]] auto is_empty() const -> bool { return m_impl == nullptr; }

	explicit operator bool() const { return !is_empty(); }

  private:
	struct Impl;
	struct Deleter {
		auto operator()(Impl* ptr) const -> void;
	};
	std::unique_ptr<Impl, Deleter> m_impl{};
};
} // namespace spaced::graphics
