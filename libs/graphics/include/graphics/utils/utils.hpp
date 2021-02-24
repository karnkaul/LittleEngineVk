#pragma once
#include <map>
#include <core/io/reader.hpp>
#include <core/os.hpp>
#include <graphics/context/physical_device.hpp>
#include <graphics/context/queue_multiplex.hpp>
#include <graphics/descriptor_set.hpp>
#include <graphics/geometry.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/shader.hpp>
#include <graphics/texture.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>
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
	std::map<set_t, kt::fixed_vector<BindingInfo, 16>> sets;
	std::vector<vk::PushConstantRange> push;
};

inline std::string_view g_compiler = "glslc";

Shader::ResourcesMap shaderResources(Shader const& shader);
io::Path spirVpath(io::Path const& src, bool bDebug = levk_debug);
kt::result_t<io::Path> compileGlsl(io::Path const& src, io::Path const& dst = {}, io::Path const& prefix = {}, bool bDebug = levk_debug);
SetBindings extractBindings(Shader const& shader);

bytearray convert(std::initializer_list<u8> bytes);
bytearray convert(View<u8> bytes);
Texture::RawImage decompress(bytearray compressed, u8 channels = 4);
void release(Texture::RawImage rawImage);

using CubeImageIDs = std::array<std::string_view, 6>;
constexpr CubeImageIDs cubeImageIDs = {"right", "left", "up", "down", "front", "back"};
std::array<bytearray, 6> loadCubemap(io::Reader const& reader, io::Path const& prefix, std::string_view ext = ".jpg", CubeImageIDs const& ids = cubeImageIDs);

std::vector<QueueMultiplex::Family> queueFamilies(PhysicalDevice const& device, vk::SurfaceKHR surface);
} // namespace utils
} // namespace le::graphics
