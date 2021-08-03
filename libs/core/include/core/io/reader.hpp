#pragma once
#include <optional>
#include <sstream>
#include <string_view>
#include <core/erased_ptr.hpp>
#include <core/io/path.hpp>
#include <core/span.hpp>
#include <core/std_types.hpp>

namespace le::io {
///
/// \brief Abstract base class for reading data from various IO
///
class Reader {
  public:
	Reader() noexcept;
	Reader(Reader&&) noexcept;
	Reader& operator=(Reader&&) noexcept;
	Reader(Reader const&);
	Reader& operator=(Reader const&);
	virtual ~Reader();

  public:
	///
	/// \brief Check if an `id` is present to load
	///
	[[nodiscard]] bool present(io::Path const& id) const;
	///
	/// \brief Check if an `id` is present to load, and log an error if not
	///
	[[nodiscard]] bool checkPresence(io::Path const& id) const;
	///
	/// \brief Check if `id`s are present to load, and log errors if not
	///
	[[nodiscard]] bool checkPresences(Span<io::Path const> ids) const;
	///
	/// \brief Obtain data as `std::string`
	///
	[[nodiscard]] std::optional<std::string> string(io::Path const& id) const;
	///
	/// \brief Obtain the IO medium (of the concrete class)
	///
	std::string_view medium() const;

  public:
	///
	/// \brief Mount a path on the IO medium
	/// Mounted paths are prefixed to `id`s being searched
	///
	[[nodiscard]] virtual bool mount([[maybe_unused]] io::Path path) { return false; }
	///
	/// \brief Obtain data as `bytearray` (`std::vector<std::byte>`)
	///
	[[nodiscard]] virtual std::optional<bytearray> bytes(io::Path const& id) const = 0;
	///
	/// \brief Obtain data as `std::stringstream`
	///
	[[nodiscard]] virtual std::optional<std::stringstream> sstream(io::Path const& id) const = 0;

  protected:
	std::string m_medium;

  protected:
	virtual std::optional<io::Path> findPrefixed(io::Path const& id) const = 0;
};

///
/// \brief Concrete class for filesystem IO
///
class FileReader final : public Reader {
  public:
	///
	/// \brief Obtain full path to directory containing any of anyOf sub-paths.
	/// \param leaf directory to start searching upwards from
	/// \param anyOf list of sub-paths to match against
	/// \param maxHeight maximum recursive depth
	///
	static std::optional<io::Path> findUpwards(io::Path const& leaf, Span<io::Path const> anyOf, u8 maxHeight = 10);

  public:
	FileReader() noexcept;

  public:
	///
	/// \brief Obtain fully qualified path (if `id` is found)
	///
	io::Path fullPath(io::Path const& id) const;

  public:
	///
	/// \brief Mount filesystem directory
	///
	bool mount(io::Path path) override;
	std::optional<bytearray> bytes(io::Path const& id) const override;
	std::optional<std::stringstream> sstream(io::Path const& id) const override;

  private:
	std::vector<io::Path> m_dirs;

  private:
	std::optional<io::Path> findPrefixed(io::Path const& id) const override;

  private:
	std::vector<io::Path> finalPaths(io::Path const& id) const;
};

///
/// \brief Concrete class for `.zip` IO
///
class ZIPReader final : public Reader {
  public:
	ZIPReader();

  public:
	///
	/// \brief Mount `.zip` file
	///
	bool mount(io::Path path) override;
	std::optional<bytearray> bytes(io::Path const& id) const override;
	std::optional<std::stringstream> sstream(io::Path const& id) const override;

  private:
	std::vector<io::Path> m_zips;

  private:
	std::optional<io::Path> findPrefixed(io::Path const& id) const override;
};

///
/// \brief Concrete class for Android AAsset IO
///
class AAssetReader final : public Reader {
  public:
	AAssetReader(ErasedPtr androidApp);

  public:
	std::optional<bytearray> bytes(io::Path const& id) const override;
	std::optional<std::stringstream> sstream(io::Path const& id) const override;

  private:
	ErasedPtr m_androidApp;

  private:
	std::optional<io::Path> findPrefixed(io::Path const& id) const override;
};
} // namespace le::io
