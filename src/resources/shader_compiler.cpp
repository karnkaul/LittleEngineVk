#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/utils.hpp>
#include <engine/resources/shader_compiler.hpp>
#include <resources/resources_impl.hpp>

#if defined(LEVK_SHADER_COMPILER)
#include <core/os.hpp>

namespace le::res
{
std::string const ShaderCompiler::s_tName = utils::tName<ShaderCompiler>();

ShaderCompiler::ShaderCompiler()
{
	if (!os::sysCall("{} --version", s_compiler))
	{
		LOG_E("[{}] Failed to go Online!", s_tName);
	}
	else
	{
		m_status = Status::eOnline;
		LOG_I("[{}] Online", s_tName);
	}
}

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
	if (!os::sysCall("{} {} -o {}", s_compiler, src.string(), dst.string()))
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
	dst += Shader::Impl::s_spvExt;
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
} // namespace le::res
#endif
