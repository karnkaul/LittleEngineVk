#pragma once
#include <core/io/media.hpp>
#include <core/os.hpp>
#include <graphics/bitmap.hpp>
#include <graphics/common.hpp>
#include <graphics/context/physical_device.hpp>
#include <graphics/context/queue_multiplex.hpp>
#include <graphics/draw_view.hpp>
#include <graphics/geometry.hpp>
#include <graphics/render/descriptor_set.hpp>
#include <graphics/render/pipeline_spec.hpp>
#include <graphics/texture.hpp>
#include <ktl/fixed_vector.hpp>
#include <spirv_cross.hpp>
#include <map>

namespace le::graphics {
class CommandRotator;

struct ShaderResources {
	spirv_cross::ShaderResources resources;
	std::unique_ptr<spirv_cross::Compiler> compiler;
	ShaderType type{};
};

template <>
struct VertexInfoFactory<Vertex> {
	VertexInputInfo operator()(u32 binding) const;
};

namespace utils {
class STBImg : public TBitmap<Span<u8 const>> {
  public:
	explicit STBImg(ImageData compressed, u8 channels = 4);
	STBImg(STBImg&& rhs) noexcept { exchg(*this, rhs); }
	STBImg& operator=(STBImg rhs) noexcept { return (exchg(*this, rhs), *this); }
	~STBImg();

	static void exchg(STBImg& lhs, STBImg& rhs) noexcept;
};

struct BindingInfo {
	vk::DescriptorSetLayoutBinding binding;
	vk::ImageViewType imageType{};
	std::string name;
};

using set_t = u32;
struct SetBindings {
	std::map<set_t, ktl::fixed_vector<BindingInfo, 16>> sets;
	std::vector<vk::PushConstantRange> push;
};

struct ShaderModule {
	vk::ShaderModule module;
	ShaderType type{};
};

struct PipeData {
	vk::PipelineLayout layout;
	vk::RenderPass renderPass;
	vk::PipelineCache cache;
};

struct HashGen {
	std::size_t hash{};
	std::uint8_t offset{};

	constexpr operator std::size_t() const noexcept { return hash; }
};

struct Transition {
	not_null<Device*> device;
	not_null<CommandBuffer*> cb;
	vk::Image image;

	void operator()(vk::ImageLayout layout, LayoutStages const& stages = LayoutStages::allCommands()) const;
};

class DualTransition {
  public:
	using Stages = TPair<LayoutStages>;

	inline static LayoutPair const xfr_v = {vIL::eTransferSrcOptimal, vIL::eTransferDstOptimal};
	inline static Stages const ct_ac_v = {LayoutStages::colourTransfer(), LayoutStages::allCommands()};

	DualTransition(not_null<Device*> device, not_null<CommandBuffer*> cb, TPair<vk::Image> images, LayoutPair layouts = xfr_v, Stages const& stages = ct_ac_v);
	~DualTransition();

	void changeStages(LayoutStages stages) noexcept { m_stages = stages; }

  private:
	Transition m_a;
	Transition m_b;
	LayoutPair m_layouts;
	LayoutStages m_stages;
};

template <typename T>
	requires(std::is_convertible_v<T, std::size_t> || std::is_enum_v<T>)
constexpr utils::HashGen& operator<<(utils::HashGen& out, T const next);

inline std::string_view g_compiler = "glslc";

ktl::fixed_vector<ShaderResources, 4> shaderResources(Span<SpirV> modules);
io::Path spirVpath(io::Path const& src, bool bDebug = levk_debug);
std::optional<io::Path> compileGlsl(io::Path const& src, io::Path const& dst = {}, io::Path const& prefix = {}, bool bDebug = levk_debug);
SetBindings extractBindings(Span<SpirV> modules);
bool hasActiveModule(Span<ShaderModule const> modules) noexcept;
std::optional<vk::Pipeline> makeGraphicsPipeline(Device& dv, Span<ShaderModule const> sm, PipelineSpec const& sp, PipeData const& data);

Bitmap bitmap(std::initializer_list<Colour> pixels, u32 width, u32 height = 0);
Bitmap bitmap(Span<Colour const> pixels, u32 width, u32 height = 0);
void append(BmpBytes& out, Colour pixel);

using CubeImageURIs = std::array<std::string_view, 6>;
constexpr CubeImageURIs cubeImageURIs = {"right", "left", "up", "down", "front", "back"};
std::array<bytearray, 6> loadCubemap(io::Media const& media, io::Path const& prefix, std::string_view ext = ".jpg", CubeImageURIs const& ids = cubeImageURIs);

std::vector<QueueMultiplex::Family> queueFamilies(PhysicalDevice const& device, vk::SurfaceKHR surface);

inline bool canBlit(Image const& src, Image const& dst) noexcept { return src.blitFlags().test(BlitFlag::eSrc) && dst.blitFlags().test(BlitFlag::eDst); }
bool blit(not_null<VRAM*> vram, CommandBuffer cb, Image const& src, Image& out_dst, vk::Filter filter = vk::Filter::eLinear);
bool copy(not_null<VRAM*> vram, CommandBuffer cb, Image const& src, Image& out_dst);
bool blitOrCopy(not_null<VRAM*> vram, CommandBuffer cb, Image const& src, Image& out_dst, vk::Filter filter = vk::Filter::eLinear);
std::optional<Image> makeStorage(not_null<VRAM*> vram, CommandRotator const& cr, Image const& img);
std::size_t writePPM(not_null<Device*> device, Image& out_img, std::ostream& out_str);

constexpr vk::Viewport viewport(DrawViewport const& viewport) noexcept;
constexpr vk::Rect2D scissor(DrawScissor const& scissor) noexcept;

HashGen& operator<<(HashGen& out, VertexInputInfo const& vi);
HashGen& operator<<(HashGen& out, ShaderSpec const& ss);
HashGen& operator<<(HashGen& out, FixedStateSpec const& fs);
HashGen& operator<<(HashGen& out, PipelineSpec const& ps);
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

namespace utils {
template <typename T>
	requires(std::is_convertible_v<T, std::size_t> || std::is_enum_v<T>)
constexpr utils::HashGen& operator<<(utils::HashGen& out, T const next) {
	out.hash ^= (static_cast<std::size_t>(next) << out.offset++);
	if (out.offset >= sizeof(std::size_t)) { out.offset = 0; }
	return out;
}
} // namespace utils
} // namespace le::graphics
