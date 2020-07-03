#include <algorithm>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/io.hpp>
#include <core/utils.hpp>
#include <engine/gfx/shader.hpp>
#include <gfx/device.hpp>
#if defined(LEVK_SHADER_COMPILER)
#include <core/os.hpp>
#endif

namespace le::gfx
{
std::string const Shader::s_tName = utils::tName<Shader>();
std::string_view Shader::s_spvExt = ".spv";
std::string_view Shader::s_vertExt = ".vert";
std::string_view Shader::s_fragExt = ".frag";

Shader::Shader(stdfs::path id, Info info) : Asset(std::move(id))
{
	m_uImpl = std::make_unique<ShaderImpl>();
	bool const bCodeMapPopulated = std::any_of(info.codeMap.begin(), info.codeMap.end(), [&](auto const& entry) { return !entry.empty(); });
	[[maybe_unused]] bool const bCodeIDsPopulated =
		std::any_of(info.codeIDMap.begin(), info.codeIDMap.end(), [&](auto const& entry) { return !entry.empty(); });
	if (!bCodeMapPopulated)
	{
		ASSERT(bCodeIDsPopulated && info.pReader, "Invalid Shader ShaderData!");
		for (std::size_t idx = 0; idx < info.codeIDMap.size(); ++idx)
		{
			auto& id = info.codeIDMap.at(idx);
			auto const ext = extension(id);
			bool bSpv = true;
			if (ext == s_vertExt || ext == s_fragExt)
			{
#if defined(LEVK_SHADER_COMPILER)
				if (loadGlsl(info, id, (Type)idx))
				{
#if defined(LEVK_ASSET_HOT_RELOAD)
					auto onReloaded = [this, idx](File const* pFile) -> bool {
						if (!glslToSpirV(pFile->id, m_codeMap.at(idx)))
						{
							LOG_E("[{}] Failed to reload Shader!", s_tName);
							return false;
						}
						return true;
					};
					m_files.push_back(File(id, m_pReader->fullPath(id), FileMonitor::Mode::eTextContents, onReloaded));
#endif
				}
				else
				{
					LOG_E("[{}] Failed to compile GLSL code to SPIR-V!", s_tName);
					m_status = Status::eError;
					return;
				}
				bSpv = false;
#else
				id += s_spvExt;
#endif
			}
			if (bSpv)
			{
				auto [shaderShaderData, bResult] = info.pReader->getBytes(id);
				ASSERT(bResult, "Shader code missing!");
				if (!bResult)
				{
					LOG_E("[{}] [{}] Shader code missing: [{}]!", s_tName, m_id.generic_string(), id.generic_string());
					m_status = Status::eError;
					return;
				}
				info.codeMap.at(idx) = std::move(shaderShaderData);
			}
		}
	}
	m_codeMap = std::move(info.codeMap);
	loadAllSpirV();
}

Shader::~Shader()
{
	for (auto const& shader : m_uImpl->shaders)
	{
		g_device.destroy(shader);
	}
}

Asset::Status Shader::update()
{
	if (!m_uImpl)
	{
		m_status = Status::eMoved;
		return m_status;
	}
	Asset::update();
	return m_status;
}

#if defined(LEVK_ASSET_HOT_RELOAD)
void Shader::onReload()
{
	LOG_D("[{}] Reloading...", s_tName);
	loadAllSpirV();
	LOG_I("[{}] Reloaded", s_tName);
	m_status = Status::eReloaded;
}
#endif

#if defined(LEVK_SHADER_COMPILER)
bool Shader::loadGlsl(Info& out_info, stdfs::path const& id, Type type)
{
	if (ShaderCompiler::instance().status() != ShaderCompiler::Status::eOnline)
	{
		LOG_E("[{}] ShaderCompiler is Offline!", s_tName);
		return false;
	}
	m_pReader = dynamic_cast<FileReader const*>(out_info.pReader);
	ASSERT(m_pReader, "Cannot compile shaders without FileReader!");
	auto [glslCode, bResult] = m_pReader->getString(id);
	return bResult && glslToSpirV(id, out_info.codeMap.at((std::size_t)type));
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
		dstID += s_spvExt;
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
#endif

void Shader::loadAllSpirV()
{
	for (std::size_t idx = 0; idx < m_codeMap.size(); ++idx)
	{
		auto const& code = m_codeMap.at(idx);
		if (!code.empty())
		{
			g_device.destroy(m_uImpl->shaders.at(idx));
			vk::ShaderModuleCreateInfo createInfo;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<u32 const*>(code.data());
			m_uImpl->shaders.at(idx) = g_device.device.createShaderModule(createInfo);
		}
	}
	m_status = Status::eReady;
	return;
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

#if defined(LEVK_SHADER_COMPILER)
std::string const ShaderCompiler::s_tName = utils::tName<ShaderCompiler>();

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
	if (!os::sysCall("glslc {} -o {}", src.string(), dst.string()))
	{
		LOG_E("[{}] Shader compilation failed: [{}]", s_tName, src.generic_string());
		return false;
	}
	LOG_I("[{}] [{}] => [{}] compiled successfully", s_tName, src.filename().generic_string(), dst.filename().generic_string());
	return true;
}

bool ShaderCompiler::compile(stdfs::path const& src, bool bOverwrite)
{
	auto dst = src;
	dst += Shader::s_spvExt;
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
#endif
} // namespace le::gfx
