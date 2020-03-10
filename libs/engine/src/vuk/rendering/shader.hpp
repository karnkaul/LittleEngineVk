#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"

namespace le::vuk
{
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
		std::unordered_map<Type, std::filesystem::path> codeIDMap;
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
} // namespace le::vuk
