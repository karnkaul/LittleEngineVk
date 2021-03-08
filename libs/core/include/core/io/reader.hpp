#pragma once
#include <sstream>
#include <string_view>
#include <core/erased_ref.hpp>
#include <core/io/path.hpp>
#include <core/span.hpp>
#include <kt/result/result.hpp>

namespace le::io {
///
/// \brief Abstract base class for reading data from various IO
///
class Reader {
  public:
	template <typename T>
	using Result = kt::result_t<T>;

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
	[[nodiscard]] bool checkPresences(View<io::Path> ids) const;
	///
	/// \brief Obtain data as `std::string`
	///
	[[nodiscard]] Result<std::string> string(io::Path const& id) const;
	///
	/// \brief Obtain the IO medium (of the concrete class)
	///
	std::string_view medium() const;

  public:
	///
	/// \brief Mount a path on the IO medium
	/// Mounted paths are prefixed to `id`s being searched
	///
	[[nodiscard]] virtual bool mount([[maybe_unused]] io::Path path) {
		return false;
	}
	///
	/// \brief Obtain data as `bytearray` (`std::vector<std::byte>`)
	///
	[[nodiscard]] virtual Result<bytearray> bytes(io::Path const& id) const = 0;
	///
	/// \brief Obtain data as `std::stringstream`
	///
	[[nodiscard]] virtual Result<std::stringstream> sstream(io::Path const& id) const = 0;

  protected:
	std::string m_medium;

  protected:
	virtual Result<io::Path> findPrefixed(io::Path const& id) const = 0;
};

///
/// \brief Concrete class for filesystem IO
///
class FileReader final : public Reader {
  public:
	///
	/// \brief Obtain full path to directory containing any of `anyOf` `id`s.
	/// \param leaf directory to start searching upwards from
	/// \param anyOf list of `id`s to search for a match for
	/// \param maxHeight maximum recursive depth
	///
	static Result<io::Path> findUpwards(io::Path const& leaf, View<io::Path> anyOf, u8 maxHeight = 10);

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
	Result<bytearray> bytes(io::Path const& id) const override;
	Result<std::stringstream> sstream(io::Path const& id) const override;

  private:
	std::vector<io::Path> m_dirs;

  private:
	Result<io::Path> findPrefixed(io::Path const& id) const override;

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
	Result<bytearray> bytes(io::Path const& id) const override;
	Result<std::stringstream> sstream(io::Path const& id) const override;

  private:
	std::vector<io::Path> m_zips;

  private:
	Result<io::Path> findPrefixed(io::Path const& id) const override;
};

///
/// \brief Concrete class for Android AAsset IO
///
class AAssetReader final : public Reader {
  public:
	AAssetReader(ErasedRef const& pAndroidApp);

  public:
	Result<bytearray> bytes(io::Path const& id) const override;
	Result<std::stringstream> sstream(io::Path const& id) const override;

  private:
	ErasedRef m_androidApp;

  private:
	Result<io::Path> findPrefixed(io::Path const& id) const override;
};
} // namespace le::io
