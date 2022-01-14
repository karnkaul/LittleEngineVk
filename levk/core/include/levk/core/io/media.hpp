#pragma once
#include <ktl/enum_flags/enum_flags.hpp>
#include <levk/core/io/path.hpp>
#include <levk/core/log.hpp>
#include <levk/core/span.hpp>
#include <levk/core/std_types.hpp>
#include <levk/core/utils/vbase.hpp>
#include <optional>
#include <sstream>
#include <string_view>

namespace le::io {
///
/// \brief Abstract base class for reading data from various IO
///
class Media : public utils::VBase {
  public:
	enum class Flag { eRead, eWrite };
	using Flags = ktl::enum_flags<Flag>;

	///
	/// \brief Metadata about concrete class
	///
	struct Info {
		std::string_view name;
		Flags capabilities;
	};

	///
	/// \brief Obtain Info about this type
	///
	/// Concrete types should return identical objects for all their instances
	///
	virtual Info const& info() const noexcept = 0;
	///
	/// \brief Check if media supports reading
	///
	bool readable() const noexcept { return info().capabilities.test(Flag::eRead); }
	///
	/// \brief Check if media supports writing
	///
	bool writeable() const noexcept { return info().capabilities.test(Flag::eWrite); }

	///
	/// \brief Check if uri is present to load
	/// \param uri URI to check
	/// \param absent Level to log at if not present (silent if not set)
	///
	[[nodiscard]] bool present(Path const& uri, std::optional<LogLevel> absent = LogLevel::warn) const;
	///
	/// \brief Obtain data as `std::string`
	///
	[[nodiscard]] std::optional<std::string> string(Path const& uri) const;
	///
	/// \brief Mount a path on the IO medium
	/// Mounted paths are prefixed to uris being searched
	///
	[[nodiscard]] virtual bool mount([[maybe_unused]] Path path) { return false; }
	///
	/// \brief Unmount a path on the IO medium
	///
	virtual bool unmount([[maybe_unused]] Path const& path) noexcept { return false; }
	///
	/// \brief Unmount all mounted paths
	///
	virtual void clear() noexcept {}
	///
	/// \brief Obtain data as `bytearray` (`std::vector<std::byte>`)
	///
	[[nodiscard]] virtual std::optional<bytearray> bytes([[maybe_unused]] Path const& uri) const { return std::nullopt; }
	///
	/// \brief Obtain data as `std::stringstream`
	///
	[[nodiscard]] virtual std::optional<std::stringstream> sstream([[maybe_unused]] Path const& uri) const { return std::nullopt; }
	///
	/// \brief Write string to path
	///
	virtual bool write(Path const&, std::string_view, [[maybe_unused]] bool newline = true) const { return false; }
	///
	/// \brief Write bytes to path
	///
	virtual bool write(Path const&, Span<std::byte const>) const { return false; }

  private:
	virtual std::optional<Path> findPrefixed(Path const& uri) const = 0;
};
} // namespace le::io
