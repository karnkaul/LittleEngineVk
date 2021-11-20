#pragma once
#include <core/colour.hpp>
#include <core/hash.hpp>
#include <graphics/common.hpp>
#include <graphics/draw_view.hpp>
#include <graphics/geometry.hpp>
#include <graphics/render/command_pool.hpp>
#include <graphics/render/pipeline.hpp>
#include <graphics/render/renderer.hpp>
#include <graphics/render/rgba.hpp>
#include <graphics/render/surface.hpp>
#include <graphics/screen_rect.hpp>
#include <memory>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace le::graphics {
struct QuickVertexInput;
struct VertexInputCreateInfo;

enum class PFlag { eDepthTest, eDepthWrite, eAlphaBlend };
using PFlags = ktl::enum_flags<PFlag, u8>;
constexpr PFlags pflags_all = PFlags(PFlag::eDepthTest, PFlag::eDepthWrite, PFlag::eAlphaBlend);
static_assert(pflags_all.count() == 3, "Invariant violated");

class RenderContext : NoCopy {
  public:
	enum class Status { eIdle, eWaiting, eReady, eBegun, eEnded, eDrawing };

	static VertexInputInfo vertexInputOld(VertexInputCreateInfo const& info);
	static VertexInputInfo vertexInputOld(QuickVertexInput const& info);
	template <typename V = Vertex>
	static Pipeline::CreateInfo pipeInfoOld(PFlags flags = PFlags(PFlag::eDepthTest) | PFlag::eDepthWrite, f32 wire = 0.0f);

	RenderContext(not_null<VRAM*> vram, Swapchain::CreateInfo const& info, glm::vec2 fbSize);

	template <typename T, typename... Args>
	void makeRenderer(Args&&... args);

	Pipeline makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo info);

	bool ready(glm::ivec2 framebufferSize);
	bool waitForFrame();
	std::optional<RenderTarget> beginFrame();
	std::optional<CommandBuffer> beginDraw(ScreenView const& view, RGBA clear, ClearDepth depth = {1.0f, 0});
	bool endDraw();
	bool endFrame();
	bool submitFrame();

	void reconstruct(std::optional<graphics::Vsync> vsync = std::nullopt);

	Status status() const noexcept { return m_storage.status; }
	std::size_t index() const noexcept { return m_storage.renderer->index(); }
	Buffering buffering() const noexcept { return m_storage.renderer->buffering(); }
	Extent2D extent() const noexcept { return m_swapchain.display().extent; }
	vk::SurfaceFormatKHR swapchainFormat() const noexcept { return m_swapchain.colourFormat(); }
	Vsync vsync() const noexcept { return m_swapchain.vsync(); }
	vk::Format colourImageFormat() const noexcept;
	ColourCorrection colourCorrection() const noexcept;
	f32 aspectRatio() const noexcept;
	glm::mat4 preRotate() const noexcept;
	vk::Viewport viewport(Extent2D extent = {0, 0}, ScreenView const& view = {}, glm::vec2 depth = {0.0f, 1.0f}) const noexcept;
	vk::Rect2D scissor(Extent2D extent = {0, 0}, ScreenView const& view = {}) const noexcept;
	bool supported(Vsync vsync) const noexcept { return m_swapchain.supportedVsync().test(vsync); }

	ARenderer& renderer() const noexcept { return *m_storage.renderer; }

  private:
	void initRenderer();

	template <typename... T>
	bool check(T... any) noexcept {
		return ((m_storage.status == any) || ...);
	}
	void set(Status s) noexcept { m_storage.status = s; }
	enum Flag { eRecreate = 1 << 0, eVsync = 1 << 1 };
	struct Storage {
		std::unique_ptr<ARenderer> renderer;
		std::optional<RenderTarget> drawing;
		Deferred<vk::PipelineCache> pipelineCache;
		Status status = {};
		struct {
			std::optional<graphics::Vsync> vsync;
			bool trigger = false;
		} reconstruct;
	};

	Swapchain m_swapchain;
	Storage m_storage;
	CommandPool m_pool;
	not_null<Device*> m_device;
};

struct RenderBegin {
	RGBA clear;
	ClearDepth depth = {1.0f, 0};
	ScreenView view;
};

using CmdBufs = ktl::fixed_vector<vk::CommandBuffer, 8>;

class IDrawer {
  public:
	virtual void draw(CommandBuffer cb) = 0;
};

class ImageCache {
  public:
	using CreateInfo = Image::CreateInfo;

	ImageCache() = default;
	ImageCache(not_null<VRAM*> vram, CreateInfo const& info = {}) noexcept : m_vram(vram) { setInfo(info); }

	bool ready() const noexcept { return m_vram != nullptr; }

	void setInfo(CreateInfo const& info) { m_info = std::move(info); }
	CreateInfo& setDepth();
	CreateInfo& setColour();

	bool ready(Extent2D extent, vk::Format format) const noexcept;
	Image& make(Extent2D extent, vk::Format format);
	Image& refresh(Extent2D extent, vk::Format format);

  private:
	Image::CreateInfo m_info;
	std::optional<Image> m_image;
	VRAM* m_vram{};
};

class Renderer {
  public:
	using Transition = rtech::Transition;
	using Approach = rtech::Approach;
	using Target = rtech::Target;
	using Tech = rtech::Tech;

	struct Attachment {
		LayoutPair layouts;
		vk::Format format;
	};

	struct CreateInfo;

	static vk::Viewport viewport(Extent2D extent = {0, 0}, ScreenView const& view = {}, glm::vec2 depth = {0.0f, 1.0f}) noexcept;
	static vk::Rect2D scissor(Extent2D extent = {0, 0}, ScreenView const& view = {}) noexcept;
	static constexpr Extent2D scaleExtent(Extent2D extent, f32 scale) noexcept;

	Renderer(CreateInfo const& info);
	virtual ~Renderer() = default;

	CmdBufs render(IDrawer& out_drawer, RenderImage const& acquired, RenderBegin const& rb);

	Tech tech() const noexcept { return Tech{Approach::eForward, m_target, m_transition}; }
	bool canScale() const noexcept;
	f32 renderScale() const noexcept { return m_scale; }
	bool renderScale(f32) noexcept;

	virtual vk::RenderPass renderPass() const noexcept { return m_singleRenderPass; }

  protected:
	Deferred<vk::Framebuffer> makeFramebuffer(vk::RenderPass rp, Span<vk::ImageView const> views, Extent2D extent, u32 layers = 1) const;
	Deferred<vk::RenderPass> makeRenderPass(Transition transition, vk::Format colour = {}, std::optional<vk::Format> depth = std::nullopt) const;

	virtual void doRender(IDrawer& out_drawer, RenderImage const& acquired, RenderBegin const& rb);
	virtual void next();

	ImageCache m_depthImage;
	ImageCache m_colourImage;
	vk::Format m_colourFormat = vk::Format::eR8G8B8A8Unorm;
	not_null<VRAM*> m_vram;

  private:
	struct Cmd {
		Deferred<vk::CommandPool> pool;
		CommandBuffer cb;

		static Cmd make(not_null<Device*> device);
	};

	using Cmds = ktl::fixed_vector<Cmd, 8>;

	RingBuffer<Cmds> m_cmds;
	Deferred<vk::RenderPass> m_singleRenderPass;
	Surface::Format m_surfaceFormat;
	Transition m_transition;
	Target m_target;
	f32 m_scale = 1.0f;
};

struct Renderer::CreateInfo {
	VRAM* vram{};
	Surface::Format format;
	Transition transition = Transition::eCommandBuffer;
	Target target = Target::eSwapchain;
	Buffering buffering = 2_B;
	u8 cmdPerFrame = 1;
};

namespace foo {
class RenderContext : public NoCopy {
  public:
	using Acquire = Surface::Acquire;
	using Attachment = Renderer::Attachment;

	static VertexInputInfo vertexInput(VertexInputCreateInfo const& info);
	static VertexInputInfo vertexInput(QuickVertexInput const& info);
	template <typename V = Vertex>
	static Pipeline::CreateInfo pipeInfo(PFlags flags = PFlags(PFlag::eDepthTest) | PFlag::eDepthWrite, f32 wire = 0.0f);
	static Deferred<vk::RenderPass> makeRenderPass(Device& device, Attachment colour, Attachment depth, vAP<vk::SubpassDependency> deps = {});

	RenderContext(not_null<VRAM*> vram, std::optional<VSync> vsync, Extent2D fbSize, Buffering buffering = 2_B);

	std::unique_ptr<Renderer> defaultRenderer();
	void setRenderer(std::unique_ptr<Renderer>&& renderer) noexcept { m_renderer = std::move(renderer); }

	Pipeline makePipeline(std::string_view id, Shader const& shader, Pipeline::CreateInfo info);

	void waitForFrame();
	bool render(IDrawer& out_drawer, RenderBegin const& rb, Extent2D fbSize);
	bool recreateSwapchain(Extent2D fbSize, std::optional<VSync> vsync);

	Buffering buffering() const noexcept { return m_buffering; }
	Extent2D extent() const noexcept { return m_surface.extent(); }
	Surface::Format surfaceFormat() const noexcept { return m_surface.format(); }
	VSync vsync() const noexcept { return m_surface.format().vsync; }
	vk::Format colourImageFormat() const noexcept;
	ColourCorrection colourCorrection() const noexcept;
	f32 aspectRatio() const noexcept;
	bool supported(VSync vsync) const noexcept { return m_surface.vsyncs().test(vsync); }

	Renderer& renderer() const noexcept { return *m_renderer; }

	struct Sync;

  private:
	bool submit(Span<vk::CommandBuffer const> cbs, Acquire const& acquired, Extent2D fbSize);

	Surface m_surface;
	RingBuffer<Sync> m_syncs;
	Deferred<vk::PipelineCache> m_pipelineCache;
	not_null<VRAM*> m_vram;
	std::unique_ptr<Renderer> m_renderer;
	Buffering m_buffering;
};

struct RenderContext::Sync {
	static Sync make(not_null<Device*> device) {
		Sync ret;
		ret.draw = makeDeferred<vk::Semaphore>(device);
		ret.present = makeDeferred<vk::Semaphore>(device);
		ret.drawn = makeDeferred<vk::Fence>(device, true);
		return ret;
	}

	Deferred<vk::Semaphore> draw;
	Deferred<vk::Semaphore> present;
	Deferred<vk::Fence> drawn;
};
} // namespace foo

struct VertexInputCreateInfo {
	struct Member {
		vk::Format format = vk::Format::eR32G32B32Sfloat;
		std::size_t offset = 0;
	};
	struct Type {
		std::vector<Member> members;
		std::size_t size = 0;
		vk::VertexInputRate inputRate = vk::VertexInputRate::eVertex;
	};
	std::vector<Type> types;
	std::size_t bindStart = 0;
	std::size_t locationStart = 0;
};

struct QuickVertexInput {
	struct Attribute {
		vk::Format format = vk::Format::eR32G32B32Sfloat;
		std::size_t offset = 0;
	};

	u32 binding = 0;
	std::size_t size = 0;
	std::vector<Attribute> attributes;
};

// impl

template <typename V>
Pipeline::CreateInfo foo::RenderContext::pipeInfo(PFlags flags, f32 wire) {
	Pipeline::CreateInfo ret;
	ret.fixedState.vertexInput = VertexInfoFactory<V>()(0);
	if (flags.test(PFlag::eDepthTest)) {
		ret.fixedState.depthStencilState.depthTestEnable = true;
		ret.fixedState.depthStencilState.depthCompareOp = vk::CompareOp::eLess;
	}
	if (flags.test(PFlag::eDepthWrite)) { ret.fixedState.depthStencilState.depthWriteEnable = true; }
	if (flags.test(PFlag::eAlphaBlend)) {
		using CCF = vk::ColorComponentFlagBits;
		ret.fixedState.colorBlendAttachment.colorWriteMask = CCF::eR | CCF::eG | CCF::eB | CCF::eA;
		ret.fixedState.colorBlendAttachment.blendEnable = true;
		ret.fixedState.colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		ret.fixedState.colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		ret.fixedState.colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
		ret.fixedState.colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
		ret.fixedState.colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		ret.fixedState.colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
	}
	if (wire > 0.0f) {
		ret.fixedState.rasterizerState.polygonMode = vk::PolygonMode::eLine;
		ret.fixedState.rasterizerState.lineWidth = wire;
	}
	return ret;
}

template <typename T, typename... Args>
void RenderContext::makeRenderer(Args&&... args) {
	ENSURE(m_storage.status < Status::eReady, "Invalid RenderContext status");
	m_storage.renderer = std::make_unique<T>(&m_swapchain, std::forward<Args>(args)...);
	initRenderer();
}

inline f32 RenderContext::aspectRatio() const noexcept {
	glm::ivec2 const ext = extent();
	return f32(ext.x) / std::max(f32(ext.y), 1.0f);
}
inline ColourCorrection RenderContext::colourCorrection() const noexcept {
	return Swapchain::srgb(swapchainFormat().format) ? ColourCorrection::eAuto : ColourCorrection::eNone;
}
inline vk::Format RenderContext::colourImageFormat() const noexcept {
	return colourCorrection() == ColourCorrection::eAuto ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Snorm;
}

constexpr Extent2D Renderer::scaleExtent(Extent2D extent, f32 scale) noexcept {
	glm::vec2 const ret = glm::vec2(f32(extent.x), f32(extent.y)) * scale;
	return {u32(ret.x), u32(ret.y)};
}

namespace foo {
inline f32 RenderContext::aspectRatio() const noexcept {
	glm::ivec2 const ext = extent();
	return f32(ext.x) / std::max(f32(ext.y), 1.0f);
}
inline ColourCorrection RenderContext::colourCorrection() const noexcept {
	return Surface::srgb(surfaceFormat().colour.format) ? ColourCorrection::eAuto : ColourCorrection::eNone;
}
inline vk::Format RenderContext::colourImageFormat() const noexcept {
	return colourCorrection() == ColourCorrection::eAuto ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Snorm;
}
} // namespace foo
} // namespace le::graphics
