#pragma once
#include <core/io/media.hpp>

namespace le::io {
///
/// \brief Concrete class for `.zip` IO
///
/// Depends on active ZIPFS instance / service for member functions to be active
///
class ZIPMedia final : public Media {
  public:
	static constexpr Info info_v = {"ZIP", Flag::eRead};

	ZIPMedia() = default;
	ZIPMedia(ZIPMedia&&) = default;
	ZIPMedia& operator=(ZIPMedia&&) noexcept;
	~ZIPMedia() noexcept override;

	Info const& info() const noexcept override { return info_v; }

	///
	/// \brief Check if ZIPFS service is available
	///
	/// Member functions early return if this is false
	///
	static bool fsActive() noexcept;

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
	std::optional<Path> findPrefixed(Path const& uri) const override;

	std::vector<Path> m_zips;
};

///
/// \brief RAII zip filesystem init/deinit
///
/// Only one active instance required/supported
/// Registers self as service
///
class ZIPFS final {
  public:
	ZIPFS();
	~ZIPFS();

	bool active() const noexcept { return m_init; }
	explicit operator bool() const noexcept { return active(); }

  private:
	bool m_init{};
};
} // namespace le::io
