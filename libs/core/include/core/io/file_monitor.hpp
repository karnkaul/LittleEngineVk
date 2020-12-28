#pragma once
#include <filesystem>
#include <core/io/reader.hpp>

namespace le::io {
namespace stdfs = std::filesystem;

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
	enum class Status : s8 { eUpToDate, eNotFound, eModified, eCOUNT_ };

  public:
	///
	/// \brief Constructor
	/// \param path: fully qualified file path to monitor
	/// \param mode: mode to operate the monitor in
	///
	FileMonitor(stdfs::path const& path, Mode mode);
	FileMonitor(FileMonitor&&);
	FileMonitor& operator=(FileMonitor&&);
	virtual ~FileMonitor();

  public:
	///
	/// \brief Obtain current status of file being monitored
	///
	virtual Status update();

  public:
	///
	/// \brief Obtain previous status of file being monitored
	///
	Status lastStatus() const;
	///
	/// \brief Obtain write-to-file timestamp
	///
	stdfs::file_time_type lastWriteTime() const;
	///
	/// \brief Obtain last modified timestamp
	///
	stdfs::file_time_type lastModifiedTime() const;

	///
	/// \brief Obtain the file path being monitored
	///
	stdfs::path const& path() const;
	///
	/// \brief Obtain the last scanned contents of the file being monitored
	/// Note: only valid for `eTextContents` mode
	///
	std::string_view text() const;
	///
	/// \brief Obtain the last scanned contents of the file being monitored
	/// Note: only valid for `eBinaryContents` mode
	bytearray const& bytes() const;

  protected:
	inline static io::FileReader s_reader;

  protected:
	stdfs::file_time_type m_lastWriteTime = {};
	stdfs::file_time_type m_lastModifiedTime = {};
	stdfs::path m_path;
	std::string m_text;
	bytearray m_bytes;
	Mode m_mode;
	Status m_status = Status::eNotFound;
};
} // namespace le::io
