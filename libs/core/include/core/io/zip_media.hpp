#pragma once
#include <core/io/media.hpp>

namespace le::io {
///
/// \brief Concrete class for `.zip` IO
///
/// Depends on active ZIPFS instance / service for member functions to be active
///
class ZIPMedia final : public Media, public NoCopy {
  public:
	static constexpr Info info_v = {"ZIP", Flag::eRead};

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
	struct Zip {
		Path path;

		Zip(Path path);
		Zip(Zip&& rhs) noexcept { exchg(*this, rhs); }
		Zip& operator=(Zip rhs) noexcept { return (exchg(*this, rhs), *this); }
		~Zip();
		static void exchg(Zip& lhs, Zip& rhs) noexcept;

		bool operator==(Zip const&) const = default;
	};

	std::optional<Path> findPrefixed(Path const& uri) const override;

	std::vector<Zip> m_zips;
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
