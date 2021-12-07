#pragma once
#include <graphics/command_buffer.hpp>
#include <graphics/memory.hpp>
#include <graphics/render/buffering.hpp>
#include <graphics/render/rgba.hpp>
#include <graphics/render/surface.hpp>
#include <graphics/screen_rect.hpp>
#include <graphics/utils/deferred.hpp>
#include <graphics/utils/ring_buffer.hpp>

namespace le::graphics {
class PipelineFactory;

static constexpr u8 max_secondary_cmd_v = 8;

struct RenderBegin {
	RGBA clear;
	ClearDepth depth = {1.0f, 0};
	ScreenView view;
};

struct RenderInfo {
	CommandBuffer primary;
	ktl::fixed_vector<CommandBuffer, max_secondary_cmd_v> secondary;
	Framebuffer framebuffer;
	RenderBegin begin;
	vk::RenderPass pass;
};

class IDrawer {
  public:
	virtual void beginFrame() = 0;
	virtual void beginPass(PipelineFactory& pf, vk::RenderPass rp) = 0;
	virtual void draw(Span<CommandBuffer> cbs) = 0;
};

class ImageCache {
  public:
	using CreateInfo = Image::CreateInfo;

	ImageCache() = default;
	ImageCache(not_null<VRAM*> vram, CreateInfo const& info = {}) noexcept : m_vram(vram) { setInfo(info); }

	bool ready() const noexcept { return m_vram != nullptr; }

	void setInfo(CreateInfo const& info) { m_info = info; }
	CreateInfo const& info() const noexcept { return m_info; }
	CreateInfo& setDepth();
	CreateInfo& setColour();

	bool ready(Extent2D extent, vk::Format format) const noexcept;
	Image& make(Extent2D extent, vk::Format format);
	Image& refresh(Extent2D extent, vk::Format format);
	Image const* peek() const noexcept { return m_image ? &*m_image : nullptr; }

  private:
	Image::CreateInfo m_info;
	std::optional<Image> m_image;
	VRAM* m_vram{};
};

class Renderer {
  public:
	enum class Approach { eForward, eDeferred, eOther };
	enum class Target { eSwapchain, eOffScreen };

	struct Tech {
		Approach approach{};
		Target target{};
	};

	struct Attachment {
		LayoutPair layouts = {vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal};
		vk::Format format = vk::Format::eR8G8B8A8Unorm;
	};

	struct CreateInfo;

	static constexpr Extent2D scaleExtent(Extent2D extent, f32 scale) noexcept;
	static vk::Viewport viewport(Extent2D extent = {0, 0}, ScreenView const& view = {}, glm::vec2 depth = {0.0f, 1.0f}) noexcept;
	static vk::Rect2D scissor(Extent2D extent = {0, 0}, ScreenView const& view = {}) noexcept;

	static Deferred<vk::RenderPass> makeRenderPass(not_null<Device*> device, Attachment colour, Attachment depth, Span<vk::SubpassDependency const> deps);
	static Deferred<vk::RenderPass> makeSingleRenderPass(not_null<Device*> device, vk::Format colour, vk::Format depth, Span<vk::SubpassDependency const> deps);
	static void execRenderPass(not_null<Device*> device, IDrawer& out_drawer, PipelineFactory& pf, RenderInfo info);

	Renderer(CreateInfo const& info);
	virtual ~Renderer() = default;

	vk::CommandBuffer render(IDrawer& out_drawer, PipelineFactory& pf, RenderTarget const& acquired, RenderBegin const& rb);

	Tech tech() const noexcept { return Tech{Approach::eForward, m_target}; }
	bool canScale() const noexcept;
	f32 renderScale() const noexcept { return m_scale; }
	bool renderScale(f32) noexcept;
	bool canBlitFrame() const noexcept { return m_blitFlags.test(BlitFlag::eSrc); }

	virtual vk::RenderPass renderPass() const noexcept { return m_singleRenderPass; }
	virtual Image const* offScreen() const noexcept { return m_colourImage.peek(); }

  protected:
	Deferred<vk::RenderPass> makeRenderPass(vk::Format colour = {}, std::optional<vk::Format> depth = std::nullopt,
											Span<vk::SubpassDependency const> deps = {}) const;

	virtual void doRender(IDrawer& out_drawer, PipelineFactory& pf, RenderTarget const& acquired, RenderBegin const& rb);
	virtual void next();

	ImageCache m_depthImage;
	ImageCache m_colourImage;
	vk::Format m_colourFormat = vk::Format::eR8G8B8A8Srgb;
	not_null<VRAM*> m_vram;

  private:
	struct Cmd {
		Deferred<vk::CommandPool> pool;
		CommandBuffer cb;

		static Cmd make(not_null<Device*> device, bool secondary = false);
	};

	using Cmds = ktl::fixed_vector<Cmd, max_secondary_cmd_v>;

	RingBuffer<Cmd> m_primaryCmd;
	RingBuffer<Cmds> m_secondaryCmds;
	Deferred<vk::RenderPass> m_singleRenderPass;
	Surface::Format m_surfaceFormat;
	TPair<f32> m_scaleLimits = {0.25f, 4.0f};
	Target m_target;
	BlitFlags m_blitFlags;
	f32 m_scale = 1.0f;
};

struct Renderer::CreateInfo {
	not_null<VRAM*> vram;
	Surface::Format surfaceFormat;
	BlitFlags surfaceBlitFlags;
	Target target = Target::eOffScreen;
	Buffering buffering = 2_B;
	u8 secondaryCmds = 1;

	CreateInfo(not_null<VRAM*> vram, Surface::Format const& surfaceFormat) : vram(vram), surfaceFormat(surfaceFormat) {}
};

// impl

constexpr Extent2D Renderer::scaleExtent(Extent2D extent, f32 scale) noexcept {
	glm::vec2 const ret = glm::vec2(f32(extent.x), f32(extent.y)) * scale;
	return {u32(ret.x), u32(ret.y)};
}
} // namespace le::graphics
