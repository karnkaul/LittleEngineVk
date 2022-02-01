#pragma once
#include <levk/core/io/media.hpp>

namespace le::io {
///
/// \brief Concrete class for `.zip` IO
///
class ZIPMedia final : public Media, public NoCopy {
  public:
	static constexpr Info info_v = {"ZIP", Flag::eRead};

	Info const& info() const noexcept override { return info_v; }

	///
	/// \brief Initialize ZIP filesystem if not initialized
	/// \returns true if successfully/already initialized
	///
	static bool fsInit();
	///
	/// \brief Deinitialize ZIP filesystem if initialized
	/// \returns true if successfully/already deinitialized
	///
	static bool fsDeinit();
	///
	/// \brief Check if ZIPFS is initialized
	///
	/// Member functions early return if this is false
	///
	static bool fsActive() noexcept;

	///
	/// \brief Default constructor; calls fsInit()
	///
	ZIPMedia();

	///
	/// \brief Mount .zip file at path
	///
	bool mount(Path path) override;
	///
	/// \brief Mount .zip contents through memory
	///
	bool mount(Path point, Span<std::byte const> bytes);
	bool unmount(Path const& path) noexcept override;
	void clear() noexcept override;
	std::optional<bytearray> bytes(Path const& uri) const override;
	std::optional<std::stringstream> sstream(Path const& uri) const override;

  private:
	struct Zip {
		Path path;

		Zip(Path path) noexcept : path(std::move(path)) {}
		Zip(Zip&& rhs) noexcept { exchg(*this, rhs); }
		Zip& operator=(Zip rhs) noexcept { return (exchg(*this, rhs), *this); }
		~Zip();
		static void exchg(Zip& lhs, Zip& rhs) noexcept;

		bool operator==(Zip const&) const = default;
	};

	std::optional<Path> findPrefixed(Path const& uri) const override;

	std::vector<Zip> m_zips;
};
} // namespace le::io
