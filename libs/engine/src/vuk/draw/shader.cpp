#include "core/assert.hpp"
#include "core/io.hpp"
#include "vuk/utils.hpp"
#include "shader.hpp"

namespace le::vuk
{
std::array<vk::ShaderStageFlagBits, size_t(Shader::Type::eCOUNT_)> const Shader::s_typeToFlagBit = {vk::ShaderStageFlagBits::eVertex,
																									vk::ShaderStageFlagBits::eFragment};

Shader::Shader(Data data) : m_id(std::move(data.id))
{
	if (data.codeMap.empty())
	{
		ASSERT(!data.codeIDMap.empty() && data.pReader, "Invalid Shader Data!");
		for (auto const& [type, id] : data.codeIDMap)
		{
			auto [shaderData, bResult] = data.pReader->getBytes(id);
			ASSERT(bResult, "Shader code missing!");
			data.codeMap[type] = std::move(shaderData);
		}
	}
	for (auto const& [type, code] : data.codeMap)
	{
		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<u32 const*>(code.data());
		m_shaders[type] = g_info.device.createShaderModule(createInfo);
	}
}

Shader::~Shader()
{
	for (auto const& [type, shader] : m_shaders)
	{
		vkDestroy(shader);
	}
}

vk::ShaderModule const* Shader::module(Type type) const
{
	if (auto search = m_shaders.find(type); search != m_shaders.end())
	{
		return &search->second;
	}
	ASSERT(false, "Module not present in Shader!");
	return nullptr;
}

std::unordered_map<Shader::Type, vk::ShaderModule> const& Shader::modules() const
{
	return m_shaders;
}
} // namespace le::vuk
