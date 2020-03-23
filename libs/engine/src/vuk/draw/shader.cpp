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
std::string const Shader::s_tName = utils::tName<Shader>();
std::array<vk::ShaderStageFlagBits, size_t(Shader::Type::eCOUNT_)> const Shader::s_typeToFlagBit = {vk::ShaderStageFlagBits::eVertex,
																									vk::ShaderStageFlagBits::eFragment};

Shader::Shader(Data data) : m_id(std::move(data.id))
{
	if (data.codeMap.empty())
	{
		ASSERT(!data.codeIDMap.empty() && data.pReader, "Invalid Shader Data!");
		for (auto const& [type, id] : data.codeIDMap)
		{
			auto const ext = extension(id);
			if (ext == ".vert" || ext == ".frag")
			{
				if (!loadGlsl(data, id, type))
				{
					LOG_E("[{}] Failed to compile GLSL code to SPIR-V!", s_tName);
				}
			}
			else if (ext == ".spv")
			{
				auto [shaderData, bResult] = data.pReader->getBytes(id);
				ASSERT(bResult, "Shader code missing!");
				data.codeMap[type] = std::move(shaderData);
			}
			else
			{
				LOG_E("[{}] Unknown extension! [{}]", s_tName, ext);
			}
		}
	}
	loadSpirV(data.codeMap);
}

Shader::~Shader()
{
	for (auto const& shader : m_shaders)
	{
		vkDestroy(shader);
	}
}

vk::ShaderModule Shader::module(Type type) const
{
	ASSERT(m_shaders.at((size_t)type) != vk::ShaderModule(), "Module not present in Shader!");
	return m_shaders.at((size_t)type);
}

std::unordered_map<Shader::Type, vk::ShaderModule> Shader::modules() const
{
	std::unordered_map<Type, vk::ShaderModule> ret;
	for (size_t idx = 0; idx < (size_t)Type::eCOUNT_; ++idx)
	{
		auto const& module = m_shaders.at(idx);
		if (module != vk::ShaderModule())
		{
			ret[(Type)idx] = module;
		}
	}
	return ret;
}

#if defined(LEVK_SHADER_HOT_RELOAD)
FileMonitor::Status Shader::hasReloaded()
{
	bool bDirty = false;
	for (auto& monitor : m_monitors)
	{
		if (monitor.monitor.update() == FileMonitor::Status::eModified)
		{
			bDirty = true;
		}
	}
	if (bDirty)
	{
		std::unordered_map<Type, bytearray> spvCode;
		for (auto& monitor : m_monitors)
		{
			if (!glslToSpirV(monitor.id, spvCode[monitor.type], monitor.pReader))
			{
				LOG_E("[{}] Failed to reload Shader!", s_tName);
				return FileMonitor::Status::eUpToDate;
			}
		}
		LOG_D("[{}] Reloading...", s_tName);
		for (auto shader : m_shaders)
		{
			vkDestroy(shader);
		}
		loadSpirV(spvCode);
		LOG_I("[{}] Reloaded", s_tName);
		return FileMonitor::Status::eModified;
	}
	return FileMonitor::Status::eUpToDate;
}
#endif

bool Shader::loadGlsl(Data& out_data, stdfs::path const& id, Type type)
{
	if (ShaderCompiler::instance().status() != ShaderCompiler::Status::eOnline)
	{
		LOG_E("[{}] ShaderCompiler is Offline!", s_tName);
		return false;
	}
	auto pFileReader = dynamic_cast<FileReader const*>(out_data.pReader);
	ASSERT(pFileReader, "Cannot compile shaders without FileReader!");
	auto [glslCode, bResult] = pFileReader->getString(id);
	if (bResult)
	{
		if (glslToSpirV(id, out_data.codeMap[type], pFileReader))
		{
#if defined(LEVK_SHADER_HOT_RELOAD)
			m_monitors.push_back({id, FileMonitor(pFileReader->fullPath(id), FileMonitor::Mode::eContents), type, pFileReader});
#endif
			return true;
		}
	}
	return false;
}

bool Shader::glslToSpirV(stdfs::path const& id, bytearray& out_bytes, FileReader const* pReader)
{
	if (ShaderCompiler::instance().status() != ShaderCompiler::Status::eOnline)
	{
		LOG_E("[{}] ShaderCompiler is Offline!", s_tName);
		return false;
	}
	ASSERT(pReader, "Cannot compile shaders without FileReader!");
	auto [glslCode, bResult] = pReader->getString(id);
	if (bResult)
	{
		auto const src = pReader->fullPath(id);
		auto dstID = id;
		dstID += ShaderCompiler::s_extension;
		if (!ShaderCompiler::instance().compile(src, true))
		{
			return false;
		}
		auto [spvCode, bResult] = pReader->getBytes(dstID);
		if (!bResult)
		{
			return false;
		}
		out_bytes = std::move(spvCode);
		return true;
	}
	return false;
}

void Shader::loadSpirV(std::unordered_map<Type, bytearray> const& byteMap)
{
	std::array<vk::ShaderModule, (size_t)Type::eCOUNT_> newModules;
	for (auto const& [type, code] : byteMap)
	{
		vk::ShaderModuleCreateInfo createInfo;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<u32 const*>(code.data());
		newModules.at((size_t)type) = g_info.device.createShaderModule(createInfo);
	}
	m_shaders = newModules;
}

std::string Shader::extension(stdfs::path const& id)
{
	auto const str = id.generic_string();
	if (auto idx = str.find_last_of('.'); idx != std::string::npos)
	{
		return str.substr(idx);
	}
	return {};
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
		m_status = Status::eOnline;
		LOG_I("[{}] Online", s_tName);
	}
}

ShaderCompiler::~ShaderCompiler() = default;

ShaderCompiler::Status ShaderCompiler::status() const
{
	return m_status;
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
	if (m_status != Status::eOnline)
	{
		LOG_E("[{}] Not Online!", s_tName);
		return false;
	}
	return true;
}
} // namespace le::vuk
