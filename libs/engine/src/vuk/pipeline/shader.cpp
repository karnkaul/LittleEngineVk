#include <fmt/format.h>
#include "core/assert.hpp"
#include "engine/vuk/pipeline/shader.hpp"
#include "vuk/instance/instance_impl.hpp"

namespace le::vuk
{
std::array<vk::ShaderStageFlagBits, size_t(Shader::Type::eCOUNT_)> const Shader::s_typeToFlagBit = {vk::ShaderStageFlagBits::eVertex,
																									vk::ShaderStageFlagBits::eFragment};

Shader::Shader(Data data) : m_id(std::move(data.id))
{
	ASSERT(g_pDevice, "Device is null!");
	auto const device = static_cast<vk::Device>(*vuk::g_pDevice);
	if (data.codeMap.empty())
	{
		ASSERT(!data.codeIDMap.empty() && data.pReader, "Invalid Shader Data!");
		for (auto const& [type, id] : data.codeIDMap)
		{
			auto [result, shaderData] = data.pReader->getBytes(id);
			ASSERT(result == IOReader::Code::eSuccess, "Shader code missing!");
			data.codeMap[type] = std::move(shaderData);
		}
	}
	for (auto const& [type, code] : data.codeMap)
	{
		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<u32 const*>(code.data());
		m_shaders[type] = device.createShaderModule(createInfo);
	}
}

Shader::~Shader()
{
	auto const device = static_cast<vk::Device>(*vuk::g_pDevice);
	for (auto const& [type, shader] : m_shaders)
	{
		device.destroy(shader);
	}
}

vk::ShaderModule const* Shader::module(Type type) const
{
	if (auto search = m_shaders.find(type); search != m_shaders.end())
	{
		return &search->second;
	}
	ASSERT(false, fmt::format("{} Module not present in Shader!", type));
	return nullptr;
}

std::unordered_map<Shader::Type, vk::ShaderModule> const& Shader::modules() const
{
	return m_shaders;
}
} // namespace le::vuk
