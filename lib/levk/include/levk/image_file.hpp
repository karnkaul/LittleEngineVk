#pragma once
#include <levk/bitmap.hpp>
#include <memory>

namespace levk {
/// \brief Image file decompresser.
class ImageFile : public IBitmapViewSource {
  public:
	/// \brief Attempt to load a compressed image.
	/// \param compressed Bytestream representing image data.
	/// \returns true on success.
	auto load_from_bytes(std::span<std::byte const> compressed) -> bool;

	/// \brief Obtain a view into the decompressed bitmap.
	/// \returns a BitmapView into the decompressed bitmap.
	[[nodiscard]] auto get_bitmap_view() const -> BitmapView final;

	[[nodiscard]] auto is_empty() const -> bool { return m_impl == nullptr; }

	explicit operator bool() const { return !is_empty(); }

  private:
	struct Impl;
	struct Deleter {
		void operator()(Impl* ptr) const;
	};
	std::unique_ptr<Impl, Deleter> m_impl{};
};
} // namespace levk
