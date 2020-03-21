#pragma once
#include <filesystem>
#include <sstream>
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
	std::string_view medium() const;
	[[nodiscard]] bool checkPresence(stdfs::path const& id) const;
	[[nodiscard]] bool checkPresences(std::initializer_list<stdfs::path> ids) const;
	[[nodiscard]] bool checkPresences(ArrayView<stdfs::path const> ids) const;

public:
	[[nodiscard]] virtual bool isPresent(stdfs::path const& id) const = 0;
	[[nodiscard]] virtual TResult<bytearray> getBytes(stdfs::path const& id) const = 0;
	[[nodiscard]] virtual TResult<std::stringstream> getStr(stdfs::path const& id) const = 0;
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
} // namespace le
