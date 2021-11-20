#pragma once
#include <graphics/render/buffering.hpp>
#include <graphics/render/command_buffer.hpp>
#include <graphics/render/rgba.hpp>
#include <graphics/render/surface.hpp>
#include <graphics/resources.hpp>
#include <graphics/screen_rect.hpp>
#include <graphics/utils/deferred.hpp>
#include <graphics/utils/ring_buffer.hpp>

namespace le::graphics {

namespace rtech {
enum class Transition { eRenderPass, eCommandBuffer };
enum class Approach { eForward, eDeferred, eOther };
enum class Target { eSwapchain, eOffScreen };

struct Tech {
	Approach approach{};
	Target target{};
	Transition transition{};
};
} // namespace rtech

struct RenderBegin {
	RGBA clear;
	ClearDepth depth = {1.0f, 0};
	ScreenView view;
};

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
	static constexpr u8 max_cmd_per_frame_v = 8;

	using Transition = rtech::Transition;
	using Approach = rtech::Approach;
	using Target = rtech::Target;
	using Tech = rtech::Tech;
	using Record = ktl::fixed_vector<vk::CommandBuffer, max_cmd_per_frame_v>;

	struct Attachment {
		LayoutPair layouts;
		vk::Format format;
	};

	struct CreateInfo;

	static vk::Viewport viewport(Extent2D extent = {0, 0}, ScreenView const& view = {}, glm::vec2 depth = {0.0f, 1.0f}) noexcept;
	static vk::Rect2D scissor(Extent2D extent = {0, 0}, ScreenView const& view = {}) noexcept;
	static Deferred<vk::RenderPass> makeRenderPass(Device& device, Attachment colour, Attachment depth, Span<vk::SubpassDependency const> deps);
	static constexpr Extent2D scaleExtent(Extent2D extent, f32 scale) noexcept;

	Renderer(CreateInfo const& info);
	virtual ~Renderer() = default;

	Record render(IDrawer& out_drawer, RenderImage const& acquired, RenderBegin const& rb);

	Tech tech() const noexcept { return Tech{Approach::eForward, m_target, m_transition}; }
	bool canScale() const noexcept;
	f32 renderScale() const noexcept { return m_scale; }
	bool renderScale(f32) noexcept;

	virtual vk::RenderPass renderPass() const noexcept { return m_singleRenderPass; }

  protected:
	Deferred<vk::Framebuffer> makeFramebuffer(vk::RenderPass rp, Span<vk::ImageView const> views, Extent2D extent, u32 layers = 1) const;
	Deferred<vk::RenderPass> makeRenderPass(Transition transition, vk::Format colour = {}, std::optional<vk::Format> depth = std::nullopt,
											Span<vk::SubpassDependency const> deps = {}) const;

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

	using Cmds = ktl::fixed_vector<Cmd, max_cmd_per_frame_v>;

	RingBuffer<Cmds> m_cmds;
	Deferred<vk::RenderPass> m_singleRenderPass;
	Surface::Format m_surfaceFormat;
	TPair<f32> m_scaleLimits = {0.25f, 4.0f};
	Transition m_transition;
	Target m_target;
	f32 m_scale = 1.0f;
};

struct Renderer::CreateInfo {
	not_null<VRAM*> vram;
	Surface::Format surfaceFormat;
	Transition transition = Transition::eCommandBuffer;
	Target target = Target::eOffScreen;
	Buffering buffering = 2_B;
	u8 cmdPerFrame = 1;

	CreateInfo(not_null<VRAM*> vram, Surface::Format const& surfaceFormat) : vram(vram), surfaceFormat(surfaceFormat) {}
};

// impl

constexpr Extent2D Renderer::scaleExtent(Extent2D extent, f32 scale) noexcept {
	glm::vec2 const ret = glm::vec2(f32(extent.x), f32(extent.y)) * scale;
	return {u32(ret.x), u32(ret.y)};
}
} // namespace le::graphics
