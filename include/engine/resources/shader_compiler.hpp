#pragma once
#include <core/io/path.hpp>
#include <core/singleton.hpp>

#if defined(LEVK_DEBUG)
#if !defined(LEVK_SHADER_COMPILER)
///
/// \brief Enable ShaderCompiler
///
#define LEVK_SHADER_COMPILER
#endif
#endif

inline constexpr bool levk_shaderCompiler = levk_debug;

namespace le::res {
#if defined(LEVK_SHADER_COMPILER)
///
/// \brief Singleton class for compiling GLSL shaders to SPIR-V
///
class ShaderCompiler final : public Singleton<ShaderCompiler> {
  public:
	enum class Status {
		eOffline, // Unable to compile (s_compiler not present?)
		eOnline,  // Ready to compile
		eCOUNT_
	};

  public:
	static std::string const s_tName;
	///
	/// \brief Name of compiler command to invoke
	///
	inline static std::string_view s_compiler = "glslc";

  private:
	ShaderCompiler();

  public:
	///
	/// \brief Obtain Status
	///
	Status status() const;

  public:
	///
	/// \brief Compile `src` to `dst`
	///
	bool compile(io::Path const& src, io::Path const& dst, bool bOverwrite);
	///
	/// \brief Compile `src` to `src`.spv
	///
	bool compile(io::Path const& src, bool bOverwrite);

  private:
	bool statusCheck() const;

  private:
	friend class Singleton<ShaderCompiler>;

  private:
	Status m_status;
};
#endif
} // namespace le::res
