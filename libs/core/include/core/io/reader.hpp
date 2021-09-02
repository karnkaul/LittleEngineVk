#pragma once
#include <optional>
#include <sstream>
#include <string_view>
#include <core/erased_ptr.hpp>
#include <core/io/path.hpp>
#include <core/not_null.hpp>
#include <core/span.hpp>
#include <core/std_types.hpp>
#include <core/utils/vbase.hpp>

namespace le::io {
///
/// \brief Abstract base class for reading data from various IO
///
class Reader : public utils::VBase {
  public:
	///
	/// \brief Check if an `id` is present to load
	///
	[[nodiscard]] bool present(Path const& id) const;
	///
	/// \brief Check if an `id` is present to load, and log an error if not
	///
	[[nodiscard]] bool checkPresence(Path const& id) const;
	///
	/// \brief Check if `id`s are present to load, and log errors if not
	///
	[[nodiscard]] bool checkPresences(Span<Path const> ids) const;
	///
	/// \brief Obtain data as `std::string`
	///
	[[nodiscard]] std::optional<std::string> string(Path const& id) const;
	///
	/// \brief Obtain the IO medium (of the concrete class)
	///
	std::string_view medium() const noexcept { return m_medium; }

  public:
	///
	/// \brief Mount a path on the IO medium
	/// Mounted paths are prefixed to `id`s being searched
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
	[[nodiscard]] virtual std::optional<bytearray> bytes(Path const& id) const = 0;
	///
	/// \brief Obtain data as `std::stringstream`
	///
	[[nodiscard]] virtual std::optional<std::stringstream> sstream(Path const& id) const = 0;

  protected:
	std::string_view m_medium;

	virtual std::optional<Path> findPrefixed(Path const& id) const = 0;
};

///
/// \brief Concrete class for filesystem IO
///
class FileReader final : public Reader {
  public:
	FileReader() noexcept;

	///
	/// \brief Obtain full path to directory containing any of anyOf sub-paths.
	/// \param leaf directory to start searching upwards from
	/// \param anyOf list of sub-paths to match against
	/// \param maxHeight maximum recursive depth
	///
	static std::optional<Path> findUpwards(Path const& leaf, Span<Path const> anyOf, u8 maxHeight = 10);
	///
	/// \brief Write text to file
	///
	static bool write(Path const& path, std::string_view text, bool newline = true);
	///
	/// \brief Write bytes to file
	///
	static bool write(Path const& path, Span<std::byte const> bytes);

	///
	/// \brief Obtain fully qualified path (if `id` is found)
	///
	Path fullPath(Path const& id) const;

	///
	/// \brief Mount filesystem directory
	///
	bool mount(Path path) override;
	bool unmount(Path const& path) noexcept override;
	void clear() noexcept override;
	std::optional<bytearray> bytes(Path const& id) const override;
	std::optional<std::stringstream> sstream(Path const& id) const override;

  private:
	std::optional<Path> findPrefixed(Path const& id) const override;
	std::vector<Path> finalPaths(Path const& id) const;

	std::vector<Path> m_dirs;
};

class ZIPFS;

///
/// \brief Concrete class for `.zip` IO
///
class ZIPReader final : public Reader {
  public:
	ZIPReader(not_null<ZIPFS const*> zipfs) noexcept;
	ZIPReader(ZIPReader&&) = default;
	ZIPReader& operator=(ZIPReader&&) noexcept;
	~ZIPReader() noexcept override;

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
	std::optional<bytearray> bytes(Path const& id) const override;
	std::optional<std::stringstream> sstream(Path const& id) const override;

  private:
	std::optional<Path> findPrefixed(Path const& id) const override;

	std::vector<Path> m_zips;
	not_null<ZIPFS const*> m_zipfs;
};

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
