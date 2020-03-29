#pragma once
#include <array>
#include <string>
#include <vector>
#include <map>
#include "core/io.hpp"
#include "gfx/vulkan.hpp"
#include "resource.hpp"

namespace le::gfx
{
class Shader final : public Resource
{
public:
	struct Info final
	{
		std::string id;
		std::array<bytearray, (size_t)ShaderType::eCOUNT_> codeMap;
		std::array<stdfs::path, (size_t)ShaderType::eCOUNT_> codeIDMap;
		class IOReader const* pReader = nullptr;
	};

#if defined(LEVK_RESOURCES_UPDATE)
private:
	struct ShaderFile : File
	{
		ShaderType type;

		ShaderFile(stdfs::path const& id, stdfs::path const& fullPath, ShaderType type);
	};
#endif

public:
	static std::string const s_tName;
	static std::array<vk::ShaderStageFlagBits, size_t(ShaderType::eCOUNT_)> const s_typeToFlagBit;

public:
	std::string m_id;

private:
	std::array<vk::ShaderModule, size_t(ShaderType::eCOUNT_)> m_shaders;
	FileReader const* m_pReader = nullptr;

public:
	Shader(Info info);
	~Shader();

public:
	vk::ShaderModule module(ShaderType type) const;
	std::map<ShaderType, vk::ShaderModule> modules() const;

#if defined(LEVK_RESOURCES_UPDATE)
	FileMonitor::Status update() override;
#endif

private:
	bool glslToSpirV(stdfs::path const& id, bytearray& out_bytes);
	bool loadGlsl(Info& out_info, stdfs::path const& id, ShaderType type);
	void loadAllSpirV(std::array<bytearray, (size_t)ShaderType::eCOUNT_> const& byteMap);

	static std::string extension(stdfs::path const& id);
};

class ShaderCompiler final
{
public:
	enum class Status
	{
		eOffline = 0,
		eOnline
	};

public:
	static std::string const s_tName;
	static std::string_view s_extension;
	static std::string_view s_compiler;

private:
	Status m_status;

public:
	static ShaderCompiler& instance();

public:
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
