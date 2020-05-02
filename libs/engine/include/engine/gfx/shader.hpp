#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include "engine/assets/asset.hpp"

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

public:
	std::unique_ptr<struct ShaderImpl> m_uImpl;

private:
	FileReader const* m_pReader = nullptr;

public:
	Shader(stdfs::path id, Info info);
	~Shader() override;

public:
	Status update() override;

private:
	bool glslToSpirV(stdfs::path const& id, bytearray& out_bytes);
	bool loadGlsl(Info& out_info, stdfs::path const& id, Type type);
	void loadAllSpirV(std::array<bytearray, (size_t)Type::eCOUNT_> const& byteMap);

	static std::string extension(stdfs::path const& id);
};

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
	static std::string_view s_extension;
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
} // namespace le::gfx
