#pragma once
#include <filesystem>
#include <sstream>
#include <string_view>
#include "core/utils.hpp"

namespace le
{
namespace stdfs = std::filesystem;

class IOReader
{
public:
	struct FBytes
	{
	private:
		IOReader const* pReader;

	public:
		TResult<bytearray> operator()(stdfs::path const& id) const;

	private:
		friend class IOReader;
	};

	struct FStr
	{
	private:
		IOReader const* pReader;

	public:
		TResult<std::stringstream> operator()(stdfs::path const& id) const;

	private:
		friend class IOReader;
	};

protected:
	stdfs::path m_prefix;
	std::string m_medium;
	FBytes m_getBytes;
	FStr m_getStr;

public:
	explicit IOReader(stdfs::path prefix) noexcept;
	IOReader(IOReader&&) noexcept;
	IOReader& operator=(IOReader&&) noexcept;
	IOReader(IOReader const&);
	IOReader& operator=(IOReader const&);
	virtual ~IOReader();

public:
	[[nodiscard]] TResult<std::string> getString(stdfs::path const& id) const;
	[[nodiscard]] FBytes bytesFunctor() const;
	[[nodiscard]] FStr strFunctor() const;
	[[nodiscard]] bool checkPresence(stdfs::path const& id) const;
	[[nodiscard]] bool checkPresences(std::initializer_list<stdfs::path> ids) const;
	[[nodiscard]] bool checkPresences(ArrayView<stdfs::path const> ids) const;

	std::string_view medium() const;

public:
	[[nodiscard]] virtual bool isPresent(stdfs::path const& id) const = 0;
	[[nodiscard]] virtual TResult<bytearray> getBytes(stdfs::path const& id) const = 0;
	[[nodiscard]] virtual TResult<std::stringstream> getStr(stdfs::path const& id) const = 0;

protected:
	stdfs::path finalPath(stdfs::path const& id) const;
};

class FileReader : public IOReader
{
public:
	static TResult<stdfs::path> findUpwards(stdfs::path const& leaf, std::initializer_list<stdfs::path> anyOf, u8 maxHeight = 10);
	static TResult<stdfs::path> findUpwards(stdfs::path const& leaf, ArrayView<stdfs::path const> anyOf, u8 maxHeight = 10);

public:
	FileReader(stdfs::path prefix = "") noexcept;

public:
	bool isPresent(stdfs::path const& id) const override;
	TResult<bytearray> getBytes(stdfs::path const& id) const override;
	TResult<std::stringstream> getStr(stdfs::path const& id) const override;

public:
	stdfs::path fullPath(stdfs::path const& id) const;
};

class ZIPReader : public IOReader
{
protected:
	stdfs::path m_zipPath;

public:
	ZIPReader(stdfs::path zipPath, stdfs::path idPrefix = "");

public:
	bool isPresent(stdfs::path const& id) const override;
	TResult<bytearray> getBytes(stdfs::path const& id) const override;
	TResult<std::stringstream> getStr(stdfs::path const& id) const override;
};

class FileMonitor
{
public:
	enum class Mode : u8
	{
		eTimestamp = 0,
		eContents
	};

	enum class Status : u8
	{
		eUpToDate = 0,
		eNotFound,
		eModified,
		eCOUNT_
	};

protected:
	static FileReader s_reader;

protected:
	stdfs::file_time_type m_lastWriteTime = {};
	stdfs::file_time_type m_lastModifiedTime = {};
	stdfs::path m_path;
	std::string m_contents;
	Mode m_mode;
	Status m_status = Status::eNotFound;

public:
	FileMonitor(stdfs::path const& path, Mode mode);
	FileMonitor(FileMonitor&&);
	FileMonitor& operator=(FileMonitor&&);
	virtual ~FileMonitor();

public:
	virtual Status update();

public:
	Status lastStatus() const;
	stdfs::file_time_type lastWriteTime() const;
	stdfs::file_time_type lastModifiedTime() const;

	stdfs::path const& path() const;
	std::string_view contents() const;
};
} // namespace le
