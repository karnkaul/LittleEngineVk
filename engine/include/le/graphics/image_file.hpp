#pragma once
#include <le/graphics/bitmap.hpp>
#include <memory>

namespace le::graphics {
class ImageFile {
  public:
	auto decompress(std::span<std::byte const> compressed) -> bool;

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
} // namespace le::graphics
