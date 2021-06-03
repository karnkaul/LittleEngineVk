#pragma once
#include <map>
#include <core/io/reader.hpp>
#include <core/os.hpp>
#include <graphics/bitmap.hpp>
#include <graphics/common.hpp>
#include <graphics/context/physical_device.hpp>
#include <graphics/context/queue_multiplex.hpp>
#include <graphics/descriptor_set.hpp>
#include <graphics/draw_view.hpp>
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
class STBImg : public TBitmap<BMPview> {
  public:
	explicit STBImg(Bitmap::type const& compressed, u8 channels = 4);
	STBImg(STBImg&&) noexcept;
	STBImg& operator=(STBImg&&) noexcept;
	~STBImg();
};

using set_t = u32;
struct SetBindings {
	std::map<set_t, kt::fixed_vector<BindingInfo, 16>> sets;
	std::vector<vk::PushConstantRange> push;
};

inline std::string_view g_compiler = "glslc";

Shader::ResourcesMap shaderResources(Shader const& shader);
io::Path spirVpath(io::Path const& src, bool bDebug = levk_debug);
kt::result<io::Path> compileGlsl(io::Path const& src, io::Path const& dst = {}, io::Path const& prefix = {}, bool bDebug = levk_debug);
SetBindings extractBindings(Shader const& shader);

Bitmap::type bitmap(std::initializer_list<u8> bytes);
Bitmap::type bitmapPx(std::initializer_list<Colour> pixels);
void append(Bitmap::type& out, View<u8> bytes);
Bitmap::type convert(View<u8> bytes);

using CubeImageIDs = std::array<std::string_view, 6>;
constexpr CubeImageIDs cubeImageIDs = {"right", "left", "up", "down", "front", "back"};
std::array<bytearray, 6> loadCubemap(io::Reader const& reader, io::Path const& prefix, std::string_view ext = ".jpg", CubeImageIDs const& ids = cubeImageIDs);

std::vector<QueueMultiplex::Family> queueFamilies(PhysicalDevice const& device, vk::SurfaceKHR surface);

constexpr vk::Viewport viewport(DrawViewport const& viewport) noexcept;
constexpr vk::Rect2D scissor(DrawScissor const& scissor) noexcept;
} // namespace utils

// impl

constexpr vk::Viewport utils::viewport(DrawViewport const& viewport) noexcept {
	vk::Viewport ret;
	ret.minDepth = viewport.depth.x;
	ret.maxDepth = viewport.depth.y;
	auto const size = viewport.size();
	ret.width = size.x;
	ret.height = -size.y; // flip viewport about X axis
	ret.x = viewport.lt.x;
	ret.y = viewport.lt.y;
	ret.y -= ret.height;
	return ret;
}

constexpr vk::Rect2D utils::scissor(DrawScissor const& scissor) noexcept {
	vk::Rect2D ret;
	glm::vec2 const size = scissor.size();
	ret.offset.x = (s32)scissor.lt.x;
	ret.offset.y = (s32)scissor.lt.y;
	ret.extent.width = (u32)size.x;
	ret.extent.height = (u32)size.y;
	return ret;
}
} // namespace le::graphics
