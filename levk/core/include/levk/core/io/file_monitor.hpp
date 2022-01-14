#pragma once
#include <ktl/either.hpp>
#include <levk/core/io/fs_media.hpp>
#include <chrono>

namespace le::io {
using file_time = std::chrono::file_clock::time_point;

///
/// \brief Utility for monitoring filesystem files
///
class FileMonitor {
  public:
	///
	/// \brief Monitoring mode
	///
	enum class Mode : s8 { eTimestamp, eTextContents, eBinaryContents };

	///
	/// \brief Monitor status
	///
	enum class Status : s8 { eUpToDate, eNotFound, eModified };

  public:
	///
	/// \brief Constructor
	/// \param path: fully qualified file path to monitor
	/// \param mode: mode to operate the monitor in
	///
	FileMonitor(Path path, Mode mode);

  public:
	///
	/// \brief Obtain current status of file being monitored
	///
	Status update();

  public:
	///
	/// \brief Obtain previous status of file being monitored
	///
	Status lastStatus() const noexcept { return m_status; }
	///
	/// \brief Obtain write-to-file timestamp
	///
	file_time lastWriteTime() const noexcept { return m_lastWriteTime; }
	///
	/// \brief Obtain last modified timestamp
	///
	file_time lastModifiedTime() const noexcept { return m_lastModifiedTime; }

	///
	/// \brief Obtain the file path being monitored
	///
	Path const& path() const noexcept { return m_path; }
	///
	/// \brief Obtain the last scanned contents of the file being monitored
	/// Note: only valid for `eTextContents` mode
	///
	std::string_view text() const;
	///
	/// \brief Obtain the last scanned contents of the file being monitored
	/// Note: only valid for `eBinaryContents` mode
	Span<std::byte const> bytes() const;

  protected:
	inline static io::FSMedia s_fsMedia;

  protected:
	file_time m_lastWriteTime = {};
	file_time m_lastModifiedTime = {};
	ktl::either<std::string, bytearray> m_payload;
	Path m_path;
	Mode m_mode;
	Status m_status = Status::eNotFound;
};
} // namespace le::io
