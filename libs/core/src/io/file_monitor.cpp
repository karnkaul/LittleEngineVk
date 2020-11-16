#include <core/io/file_monitor.hpp>
#include <core/log.hpp>

namespace le::io {

FileMonitor::FileMonitor(stdfs::path const& path, Mode mode) : m_path(path), m_mode(mode) {
}

FileMonitor::FileMonitor(FileMonitor&&) = default;
FileMonitor& FileMonitor::operator=(FileMonitor&&) = default;
FileMonitor::~FileMonitor() = default;

FileMonitor::Status FileMonitor::update() {
	std::error_code errCode;
	if (stdfs::is_regular_file(m_path, errCode)) {
		auto const lastWriteTime = stdfs::last_write_time(m_path, errCode);
		if (errCode) {
			return m_status;
		}
		if (lastWriteTime != m_lastWriteTime || m_status == Status::eNotFound) {
			bool bDirty = m_lastWriteTime != stdfs::file_time_type();
			m_lastWriteTime = lastWriteTime;
			if (m_mode == Mode::eTextContents) {
				if (auto text = s_reader.string(m_path.generic_string())) {
					if (*text == m_text) {
						bDirty = false;
					} else {
						m_text = text.move();
						m_lastModifiedTime = m_lastWriteTime;
					}
				}
			} else if (m_mode == Mode::eBinaryContents) {
				if (auto bytes = s_reader.bytes(m_path.generic_string())) {
					if (*bytes == m_bytes) {
						bDirty = false;
					} else {
						m_bytes = bytes.move();
						m_lastModifiedTime = m_lastWriteTime;
					}
				}
			}
			m_status = bDirty ? Status::eModified : Status::eUpToDate;
		} else {
			m_status = Status::eUpToDate;
		}
	} else {
		m_status = Status::eNotFound;
	}
	return m_status;
}

FileMonitor::Status FileMonitor::lastStatus() const {
	return m_status;
}

stdfs::file_time_type FileMonitor::lastWriteTime() const {
	return m_lastWriteTime;
}

stdfs::file_time_type FileMonitor::lastModifiedTime() const {
	return m_lastModifiedTime;
}

stdfs::path const& FileMonitor::path() const {
	return m_path;
}

std::string_view FileMonitor::text() const {
	ENSURE(m_mode == Mode::eTextContents, "Monitor not in Text Contents mode!");
	if (m_mode != Mode::eTextContents) {
		logE("[{}] not monitoring file contents (only timestamp) [{}]!", utils::tName<FileReader>(), m_path.generic_string());
	}
	return m_text;
}

bytearray const& FileMonitor::bytes() const {
	ENSURE(m_mode == Mode::eBinaryContents, "Monitor not in Text Contents mode!");
	if (m_mode != Mode::eBinaryContents) {
		logE("[{}] not monitoring file contents (only timestamp) [{}]!", utils::tName<FileReader>(), m_path.generic_string());
	}
	return m_bytes;
}
} // namespace le::io
