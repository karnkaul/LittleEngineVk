#pragma once
#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "core/io.hpp"

#if defined(LEVK_DEBUG)
#if !defined(LEVK_SHADER_HOT_RELOAD)
#define LEVK_SHADER_HOT_RELOAD
#endif
#endif

namespace le::gfx
{
namespace stdfs = std::filesystem;

class Shader final
{
public:
	enum class Type : u8
	{
		eVertex = 0,
		eFragment,
		eCOUNT_
	};

#if defined(LEVK_SHADER_HOT_RELOAD)
private:
	struct Monitor
	{
		stdfs::path id;
		FileMonitor monitor;
		Type type;
		FileReader const* pReader;
	};
#endif

public:
	struct Data
	{
		std::string id;
		std::unordered_map<Type, bytearray> codeMap;
		std::unordered_map<Type, stdfs::path> codeIDMap;
		IOReader const* pReader = nullptr;
	};

public:
	static std::string const s_tName;
	static std::array<vk::ShaderStageFlagBits, size_t(Type::eCOUNT_)> const s_typeToFlagBit;

public:
	std::string m_id;

#if defined(LEVK_SHADER_HOT_RELOAD)
private:
	std::vector<Monitor> m_monitors;
#endif

private:
	std::array<vk::ShaderModule, size_t(Type::eCOUNT_)> m_shaders;

public:
	Shader(Data data);
	~Shader();

public:
	vk::ShaderModule module(Type type) const;
	std::unordered_map<Type, vk::ShaderModule> modules() const;

#if defined(LEVK_SHADER_HOT_RELOAD)
	FileMonitor::Status hasReloaded();
#endif

private:
	bool glslToSpirV(stdfs::path const& id, bytearray& out_bytes, FileReader const* pReader);
	bool loadGlsl(Data& out_data, stdfs::path const& id, Type type);
	void loadAllSpirV(std::unordered_map<Type, bytearray> const& byteMap);

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
