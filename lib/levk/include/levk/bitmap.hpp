#pragma once
#include <glm/vec2.hpp>
#include <levk/core/polymorphic.hpp>
#include <bit>
#include <span>
#include <vector>

namespace levk {
/// \brief View of a 2D bitmap.
struct BitmapView {
	/// \brief Bytestream representing bitmap data.
	std::span<std::byte const> bytes{};
	/// \brief Size of bitmap.
	glm::ivec2 extent{};
};

/// \brief Interface that produces a BitmapView.
class IBitmapViewSource : public Polymorphic {
  public:
	/// \brief Obtain a BitmapView.
	/// \returns BitmapView of stored data.
	[[nodiscard]] virtual auto get_bitmap_view() const -> BitmapView = 0;
};

/// \brief 2D Bitmap storage.
class Bitmap : public IBitmapViewSource {
  public:
	using View = BitmapView;

	/// \brief Constructor.
	/// \param bytes Bytestream representing bitmap data.
	/// \param extent Size of bitmap.
	explicit Bitmap(std::vector<std::byte> bytes, glm::ivec2 extent) : m_bytes(std::move(bytes)), m_extent(extent) {}

	/// \brief Obtain a BitmapView.
	/// \returns BitmapView of stored data.
	[[nodiscard]] auto get_bitmap_view() const -> BitmapView final { return {.bytes = std::span<std::byte const>{m_bytes}, .extent = m_extent}; }

  private:
	std::vector<std::byte> m_bytes{};
	glm::ivec2 m_extent{};
};

/// \brief Check if an integer is a power of two.
constexpr auto is_power_of_2(int const in) -> bool { return std::popcount(static_cast<std::uint32_t>(in)) == 1; }
} // namespace levk
