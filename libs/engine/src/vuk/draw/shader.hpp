#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"

namespace le::vuk
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

public:
	struct Data
	{
		std::string id;
		std::unordered_map<Type, bytearray> codeMap;
		std::unordered_map<Type, stdfs::path> codeIDMap;
		class IOReader const* pReader = nullptr;
	};

public:
	static std::array<vk::ShaderStageFlagBits, size_t(Type::eCOUNT_)> const s_typeToFlagBit;

public:
	std::string m_id;

private:
	std::unordered_map<Type, vk::ShaderModule> m_shaders;

public:
	Shader(Data data);
	~Shader();

public:
	vk::ShaderModule const* module(Type type) const;
	std::unordered_map<Type, vk::ShaderModule> const& modules() const;
};

class ShaderCompiler final
{
public:
	enum class State
	{
		eOffline = 0,
		eOnline
	};

public:
	static std::string const s_tName;
	static std::string_view s_extension;

private:
	State m_state;

public:
	static ShaderCompiler& instance();

public:
	ShaderCompiler();
	~ShaderCompiler();

public:
	State state() const;

public:
	bool compile(stdfs::path const& src, stdfs::path const& dst, bool bOverwrite);
	bool compile(stdfs::path const& src, bool bOverwrite);

private:
	bool statusCheck() const;
};
} // namespace le::vuk
