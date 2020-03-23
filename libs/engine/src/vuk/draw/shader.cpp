#include <cstdlib>
#include <fmt/format.h>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/io.hpp"
#include "core/utils.hpp"
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

std::string const ShaderCompiler::s_tName = utils::tName<ShaderCompiler>();
std::string_view ShaderCompiler::s_extension = ".spv";

ShaderCompiler& ShaderCompiler::instance()
{
	static ShaderCompiler s_instance;
	return s_instance;
}

ShaderCompiler::ShaderCompiler()
{
	if (std::system("glslc --version") != 0)
	{
		LOG_E("[{}] Failed to go Online!", s_tName);
	}
	else
	{
		m_state = State::eOnline;
		LOG_I("[{}] Online", s_tName);
	}
}

ShaderCompiler::~ShaderCompiler() = default;

ShaderCompiler::State ShaderCompiler::state() const
{
	return m_state;
}

bool ShaderCompiler::compile(stdfs::path const& src, stdfs::path const& dst, bool bOverwrite)
{
	if (!statusCheck())
	{
		return false;
	}
	if (!stdfs::is_regular_file(src))
	{
		LOG_E("[{}] Failed to find source file: [{}]", s_tName, src.generic_string());
		return false;
	}
	if (stdfs::is_regular_file(dst) && !bOverwrite)
	{
		LOG_E("[{}] Destination file exists and overwrite flag not set: [{}]", s_tName, src.generic_string());
		return false;
	}
	auto const command = fmt::format("glslc {} -o {}", src.string(), dst.string());
	if (std::system(command.data()) != 0)
	{
		LOG_E("[{}] Shader compilation failed: [{}]", src.generic_string());
		return false;
	}
	LOG_I("[{}] [{}] => [{}] compiled successfully", s_tName, src.generic_string(), dst.generic_string());
	return true;
}

bool ShaderCompiler::compile(stdfs::path const& src, bool bOverwrite)
{
	auto dst = src;
	dst += s_extension;
	return compile(src, dst, bOverwrite);
}

bool ShaderCompiler::statusCheck() const
{
	if (m_state != State::eOnline)
	{
		LOG_E("[{}] Not Online!", s_tName);
		return false;
	}
	return true;
}
} // namespace le::vuk
