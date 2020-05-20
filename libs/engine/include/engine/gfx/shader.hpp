#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include "engine/assets/asset.hpp"

#if defined(LEVK_DEBUG)
#if !defined(LEVK_SHADER_COMPILER)
#define LEVK_SHADER_COMPILER
#endif
#endif

namespace le::gfx
{
class Shader final : public Asset
{
public:
	enum class Type : u8
	{
		eVertex,
		eFragment,
		eCOUNT_
	};

	struct Info final
	{
		std::array<bytearray, (size_t)Type::eCOUNT_> codeMap;
		std::array<stdfs::path, (size_t)Type::eCOUNT_> codeIDMap;
		class IOReader const* pReader = nullptr;
	};

public:
	static std::string const s_tName;
	static std::string_view s_spvExt;
	static std::string_view s_vertExt;
	static std::string_view s_fragExt;

private:
	std::unique_ptr<struct ShaderImpl> m_uImpl;
	FileReader const* m_pReader = nullptr;

public:
	Shader(stdfs::path id, Info info);
	~Shader() override;

public:
	Status update() override;

private:
#if defined(LEVK_SHADER_COMPILER)
	bool glslToSpirV(stdfs::path const& id, bytearray& out_bytes);
	bool loadGlsl(Info& out_info, stdfs::path const& id, Type type);
#endif
	void loadAllSpirV(std::array<bytearray, (size_t)Type::eCOUNT_> const& byteMap);

	static std::string extension(stdfs::path const& id);

private:
	friend class Pipeline;
	friend class PipelineImpl;
};

#if defined(LEVK_SHADER_COMPILER)
class ShaderCompiler final
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

public:
	static ShaderCompiler& instance();

private:
	ShaderCompiler();
	~ShaderCompiler();

public:
	Status status() const;

public:
	bool compile(stdfs::path const& src, stdfs::path const& dst, bool bOverwrite);
	bool compile(stdfs::path const& src, bool bOverwrite);

private:
	bool statusCheck() const;
};
#endif
} // namespace le::gfx
