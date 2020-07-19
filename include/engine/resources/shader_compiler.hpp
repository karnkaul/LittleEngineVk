#pragma once
#include <filesystem>
#include <core/singleton.hpp>

#if defined(LEVK_DEBUG)
#if !defined(LEVK_SHADER_COMPILER)
#define LEVK_SHADER_COMPILER
#endif
#endif

namespace le
{
namespace stdfs = std::filesystem;
}

namespace le::resources
{
#if defined(LEVK_SHADER_COMPILER)
class ShaderCompiler final : public Singleton<ShaderCompiler>
{
public:
	enum class Status
	{
		eOffline,
		eOnline,
		eCOUNT_
	};

public:
	static std::string const s_tName;
	static std::string_view s_compiler;

private:
	Status m_status;

private:
	ShaderCompiler();

public:
	Status status() const;

public:
	bool compile(stdfs::path const& src, stdfs::path const& dst, bool bOverwrite);
	bool compile(stdfs::path const& src, bool bOverwrite);

private:
	bool statusCheck() const;

private:
	friend class Singleton<ShaderCompiler>;
};
#endif
} // namespace le::resources
