#pragma once
#include <map>
#include <core/os.hpp>
#include <graphics/descriptor_set.hpp>
#include <graphics/geometry.hpp>
#include <graphics/shader.hpp>
#include <spirv_cross.hpp>

inline constexpr bool levk_shaderCompiler = levk_desktopOS;

namespace le::graphics {
struct Shader::Resources {
	spirv_cross::ShaderResources resources;
	std::unique_ptr<spirv_cross::Compiler> compiler;
};

template <>
struct VertexInfoFactory<Vertex> {
	VertexInputInfo operator()(u32 binding) const;
};

namespace utils {
using set_t = u32;
struct SetBindings {
	std::map<set_t, std::vector<BindingInfo>> sets;
	std::vector<vk::PushConstantRange> push;
};

inline std::string_view g_compiler = "glslc";

Shader::ResourcesMap shaderResources(Shader const& shader);
std::optional<io::Path> compileGlsl(io::Path const& src, io::Path dst = {}, io::Path const& prefix = {}, bool bDebug = levk_debug);
SetBindings extractBindings(Shader const& shader);

RawImage decompress(bytearray compressed, u8 channels = 4);
void release(RawImage rawImage);
} // namespace utils
} // namespace le::graphics
