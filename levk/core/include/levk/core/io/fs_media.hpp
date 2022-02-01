#pragma once
#include <levk/core/io/media.hpp>

namespace le::io {
///
/// \brief Concrete class for filesystem IO
///
class FSMedia final : public Media {
  public:
	static constexpr Info info_v = {"Filesystem", Flags(Flag::eRead) | Flag::eWrite};

	///
	/// \brief Obtain full path to directory containing any of anyOf sub-paths.
	/// \param leaf directory to start searching upwards from
	/// \param anyOf list of sub-paths to match against
	/// \param maxHeight maximum recursive depth
	///
	static std::optional<Path> findUpwards(Path const& leaf, Span<Path const> anyOf, u8 maxHeight = 10);

	Info const& info() const noexcept override { return info_v; }

	///
	/// \brief Obtain fully qualified path (if uri is found on any mount paths / pwd)
	///
	Path fullPath(Path const& uri) const;
	///
	/// \brief Mount filesystem directory
	///
	bool mount(Path path) override;
	bool unmount(Path const& path) noexcept override;
	void clear() noexcept override;
	std::optional<bytearray> bytes(Path const& uri) const override;
	std::optional<std::stringstream> sstream(Path const& uri) const override;
	bool write(Path const& path, std::string_view text, bool newline = true) const override;
	bool write(Path const& path, Span<std::byte const> bytes) const override;

  private:
	std::optional<Path> findPrefixed(Path const& uri) const override;

	std::vector<Path> m_dirs;
};
} // namespace le::io
