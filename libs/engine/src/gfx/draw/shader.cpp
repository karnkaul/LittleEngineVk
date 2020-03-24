#include <algorithm>
#include <cstdlib>
#include <fmt/format.h>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/io.hpp"
#include "core/utils.hpp"
#include "gfx/utils.hpp"
#include "shader.hpp"

namespace le::gfx
{
std::string const Shader::s_tName = utils::tName<Shader>();
std::array<vk::ShaderStageFlagBits, size_t(ShaderType::eCOUNT_)> const Shader::s_typeToFlagBit = {vk::ShaderStageFlagBits::eVertex,
																								  vk::ShaderStageFlagBits::eFragment};

#if defined(LEVK_RESOURCES_UPDATE)
Shader::ShaderFile::ShaderFile(stdfs::path const& id, stdfs::path const& fullPath, ShaderType type) : File(id, fullPath), type(type) {}
#endif

Shader::Shader(Info info) : m_id(std::move(info.id))
{
	bool const bCodeMapPopulated = std::any_of(info.codeMap.begin(), info.codeMap.end(), [&](auto const& entry) { return !entry.empty(); });
	[[maybe_unused]] bool const bCodeIDsPopulated =
		std::any_of(info.codeIDMap.begin(), info.codeIDMap.end(), [&](auto const& entry) { return !entry.empty(); });
	if (!bCodeMapPopulated)
	{
		ASSERT(bCodeIDsPopulated && info.pReader, "Invalid Shader ShaderData!");
		for (size_t idx = 0; idx < info.codeIDMap.size(); ++idx)
		{
			auto const id = info.codeIDMap.at(idx);
			auto const ext = extension(id);
			auto const type = (ShaderType)idx;
			if (ext == ".vert" || ext == ".frag")
			{
				if (!loadGlsl(info, id, type))
				{
					LOG_E("[{}] Failed to compile GLSL code to SPIR-V!", s_tName);
				}
			}
			else if (ext == ".spv")
			{
				auto [shaderShaderData, bResult] = info.pReader->getBytes(id);
				ASSERT(bResult, "Shader code missing!");
				info.codeMap.at(idx) = std::move(shaderShaderData);
			}
			else
			{
				LOG_E("[{}] Unknown extension! [{}]", s_tName, ext);
			}
		}
	}
	loadAllSpirV(info.codeMap);
}

Shader::~Shader()
{
	for (auto const& shader : m_shaders)
	{
		vkDestroy(shader);
	}
}

vk::ShaderModule Shader::module(ShaderType type) const
{
	ASSERT(m_shaders.at((size_t)type) != vk::ShaderModule(), "Module not present in Shader!");
	return m_shaders.at((size_t)type);
}

std::map<ShaderType, vk::ShaderModule> Shader::modules() const
{
	std::map<ShaderType, vk::ShaderModule> ret;
	for (size_t idx = 0; idx < (size_t)ShaderType::eCOUNT_; ++idx)
	{
		auto const& module = m_shaders.at(idx);
		if (module != vk::ShaderModule())
		{
			ret[(ShaderType)idx] = module;
		}
	}
	return ret;
}

#if defined(LEVK_RESOURCES_UPDATE)
FileMonitor::Status Shader::update()
{
	auto const status = Resource::update();
	if (status != FileMonitor::Status::eUpToDate)
	{
		std::array<bytearray, (size_t)ShaderType::eCOUNT_> spvCode;
		for (auto& uFile : m_files)
		{
			auto pFile = dynamic_cast<ShaderFile*>(uFile.get());
			ASSERT(pFile, "Shader contains unknown File subclass!");
			if (pFile->monitor.lastStatus() == FileMonitor::Status::eModified)
			{
				if (!glslToSpirV(pFile->id, spvCode.at((size_t)pFile->type)))
				{
					LOG_E("[{}] Failed to reload Shader!", s_tName);
					return m_lastStatus = FileMonitor::Status::eUpToDate;
				}
			}
			else
			{
				auto spvID = pFile->id;
				spvID += ShaderCompiler::s_extension;
				auto [bytes, bResult] = m_pReader->getBytes(spvID);
				if (!bResult)
				{
					LOG_E("[{}] Failed to reload Shader!", s_tName);
					return m_lastStatus = FileMonitor::Status::eUpToDate;
				}
				spvCode.at((size_t)pFile->type) = std::move(bytes);
			}
		}
		LOG_D("[{}] Reloading...", s_tName);
		for (auto shader : m_shaders)
		{
			vkDestroy(shader);
		}
		loadAllSpirV(spvCode);
		LOG_I("[{}] Reloaded", s_tName);
	}
	return m_lastStatus;
}
#endif

bool Shader::loadGlsl(Info& out_info, stdfs::path const& id, ShaderType type)
{
	if (ShaderCompiler::instance().status() != ShaderCompiler::Status::eOnline)
	{
		LOG_E("[{}] ShaderCompiler is Offline!", s_tName);
		return false;
	}
	m_pReader = dynamic_cast<FileReader const*>(out_info.pReader);
	ASSERT(m_pReader, "Cannot compile shaders without FileReader!");
	auto [glslCode, bResult] = m_pReader->getString(id);
	if (bResult)
	{
		if (glslToSpirV(id, out_info.codeMap.at((size_t)type)))
		{
#if defined(LEVK_RESOURCES_UPDATE)
			auto uFile = std::make_unique<ShaderFile>(id, m_pReader->fullPath(id), type);
			m_files.push_back(std::move(uFile));
#endif
			return true;
		}
	}
	return false;
}

bool Shader::glslToSpirV(stdfs::path const& id, bytearray& out_bytes)
{
	if (ShaderCompiler::instance().status() != ShaderCompiler::Status::eOnline)
	{
		LOG_E("[{}] ShaderCompiler is Offline!", s_tName);
		return false;
	}
	ASSERT(m_pReader, "Cannot compile shaders without FileReader!");
	auto [glslCode, bResult] = m_pReader->getString(id);
	if (bResult)
	{
		auto const src = m_pReader->fullPath(id);
		auto dstID = id;
		dstID += ShaderCompiler::s_extension;
		if (!ShaderCompiler::instance().compile(src, true))
		{
			return false;
		}
		auto [spvCode, bResult] = m_pReader->getBytes(dstID);
		if (!bResult)
		{
			return false;
		}
		out_bytes = std::move(spvCode);
		return true;
	}
	return false;
}

void Shader::loadAllSpirV(std::array<bytearray, (size_t)ShaderType::eCOUNT_> const& byteMap)
{
	std::array<vk::ShaderModule, (size_t)ShaderType::eCOUNT_> newModules;
	for (size_t idx = 0; idx < byteMap.size(); ++idx)
	{
		auto const& code = byteMap.at(idx);
		if (!code.empty())
		{
			vk::ShaderModuleCreateInfo createInfo;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<u32 const*>(code.data());
			newModules.at(idx) = g_info.device.createShaderModule(createInfo);
		}
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
} // namespace le::gfx
