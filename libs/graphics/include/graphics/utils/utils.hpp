#pragma once
#include <map>
#include <core/io/reader.hpp>
#include <core/os.hpp>
#include <graphics/bitmap.hpp>
#include <graphics/common.hpp>
#include <graphics/context/physical_device.hpp>
#include <graphics/context/queue_multiplex.hpp>
#include <graphics/draw_view.hpp>
#include <graphics/geometry.hpp>
#include <graphics/render/descriptor_set.hpp>
#include <graphics/render/pipeline.hpp>
#include <graphics/shader.hpp>
#include <graphics/texture.hpp>
#include <ktl/fixed_vector.hpp>
#include <spirv_cross.hpp>

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
class STBImg : public TBitmap<Span<u8>> {
  public:
	explicit STBImg(Bitmap::type const& compressed, u8 channels = 4);
	STBImg(STBImg&&) noexcept;
	STBImg& operator=(STBImg&&) noexcept;
	~STBImg();
};

using set_t = u32;
struct SetBindings {
	std::map<set_t, ktl::fixed_vector<BindingInfo, 16>> sets;
	std::vector<vk::PushConstantRange> push;
};

inline std::string_view g_compiler = "glslc";

Shader::ResourcesMap shaderResources(Shader const& shader);
io::Path spirVpath(io::Path const& src, bool bDebug = levk_debug);
std::optional<io::Path> compileGlsl(io::Path const& src, io::Path const& dst = {}, io::Path const& prefix = {}, bool bDebug = levk_debug);
SetBindings extractBindings(Shader const& shader);

Bitmap bitmap(std::initializer_list<Colour> pixels, u32 width, u32 height = 0);
Bitmap bitmap(Span<Colour const> pixels, u32 width, u32 height = 0);
void append(BmpBytes& out, Colour pixel);
void append(BmpBytes& out, Span<std::byte const> bytes);
BmpBytes bmpBytes(Span<std::byte const> bytes);

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
	ret.offset.x = std::max(0, (s32)scissor.lt.x);
	ret.offset.y = std::max(0, (s32)scissor.lt.y);
	ret.extent.width = std::max(0U, (u32)size.x);
	ret.extent.height = std::max(0U, (u32)size.y);
	return ret;
}
} // namespace le::graphics
