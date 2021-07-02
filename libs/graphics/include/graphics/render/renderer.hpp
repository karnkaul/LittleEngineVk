#pragma once
#include <type_traits>
#include <graphics/context/vram.hpp>
#include <graphics/render/buffering.hpp>
#include <graphics/render/command_buffer.hpp>
#include <graphics/render/fence.hpp>
#include <graphics/render/frame_drawer.hpp>
#include <graphics/render/rgba.hpp>
#include <graphics/screen_rect.hpp>

namespace le::graphics {
class Swapchain;

class ARenderer;

template <typename Rd>
concept concrete_renderer = std::is_base_of_v<ARenderer, Rd> && !std::is_same_v<ARenderer, Rd>;

class ARenderer {
  public:
	enum class Approach { eForward, eDeferred, eOther };
	enum class Target { eSwapchain, eOffScreen };

	struct Technique {
		Approach approach = Approach::eOther;
		Target target = Target::eSwapchain;
	};

	struct Attachment {
		LayoutPair layouts;
		vk::Format format;
	};

	struct Draw {
		RenderTarget target;
		CommandBuffer commandBuffer;
	};

	struct Cmd;

	ARenderer(not_null<Swapchain*> swapchain, Buffering buffering, Extent2D extent, vk::Format depthFormat);
	virtual ~ARenderer() = default;

	static constexpr Extent2D extent2D(vk::Extent3D extent) { return {extent.width, extent.height}; }
	static RenderImage renderImage(Image const& image) noexcept { return {image.image(), image.view(), cast(image.extent())}; }
	static vk::Viewport viewport(Extent2D extent, ScreenRect const& nRect = {}, glm::vec2 offset = {}, glm::vec2 depth = {0.0f, 1.0f}) noexcept;
	static vk::Rect2D scissor(Extent2D extent, ScreenRect const& nRect = {}, glm::vec2 offset = {}) noexcept;

	bool hasDepthImage() const noexcept { return m_depth.has_value(); }
	std::size_t index() const noexcept { return m_fence.index(); }
	Buffering buffering() const noexcept { return m_fence.buffering(); }

	std::optional<RenderImage> depthImage(vk::Format depthFormat, Extent2D extent);
	RenderSemaphore makeSemaphore() const;
	vk::RenderPass makeRenderPass(Attachment colour, Attachment depth, vAP<vk::SubpassDependency> deps = {}) const;
	vk::Framebuffer makeFramebuffer(vk::RenderPass renderPass, vAP<vk::ImageView> attachments, Extent2D extent, u32 layers = 1) const;

	virtual Technique technique() const noexcept = 0;
	virtual vk::RenderPass renderPass3D() const noexcept = 0;
	virtual vk::RenderPass renderPassUI() const noexcept = 0;

	virtual std::optional<Draw> beginFrame() = 0;
	virtual void beginDraw(RenderTarget const&, FrameDrawer&, RGBA, vk::ClearDepthStencilValue) = 0;
	virtual void endDraw(RenderTarget const&) = 0;
	virtual void endFrame() = 0;
	virtual bool submitFrame() = 0;

	void refresh() { m_fence.refresh(); }
	void waitForFrame();

	struct {
		ScreenRect rect;
		glm::vec2 offset{};
	} m_viewport;

	f32 m_scale = 1.0f;
	not_null<Swapchain*> m_swapchain;
	not_null<Device*> m_device;

  protected:
	RenderFence m_fence;
	std::optional<Image> m_depth;
};

struct ARenderer::Cmd {
	Cmd() = default;
	Cmd(not_null<Device*> device, vk::CommandPoolCreateFlags flags = {}, QType qtype = QType::eGraphics) {
		pool = makeDeferred<vk::CommandPool>(device, flags, qtype);
		cb = CommandBuffer::make(device, *pool, 1).front();
		draw = makeDeferred<vk::Semaphore>(device);
		present = makeDeferred<vk::Semaphore>(device);
	}

	CommandBuffer cb;
	Deferred<vk::CommandPool> pool;
	Deferred<vk::Semaphore> draw;
	Deferred<vk::Semaphore> present;
};
} // namespace le::graphics
