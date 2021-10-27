#pragma once
#include <type_traits>
#include <graphics/context/vram.hpp>
#include <graphics/render/buffering.hpp>
#include <graphics/render/command_buffer.hpp>
#include <graphics/render/fence.hpp>
#include <graphics/render/rgba.hpp>
#include <graphics/render/swapchain.hpp>
#include <graphics/screen_rect.hpp>
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

class Swapchain;

class ARenderer;

template <typename Rd>
concept concrete_renderer = std::is_base_of_v<ARenderer, Rd> && !std::is_same_v<ARenderer, Rd>;

struct ImageMaker {
	ktl::fixed_vector<Image::CreateInfo, 8> infos;
	not_null<VRAM*> vram;

	ImageMaker(not_null<VRAM*> vram) noexcept : vram(vram) {}

	bool ready(std::optional<Image>& out, Extent2D extent, vk::Format format, std::size_t idx) const noexcept;
	Image make(Extent2D extent, vk::Format format, std::size_t idx);
	Image& refresh(std::optional<Image>& out, Extent2D extent, vk::Format format, std::size_t idx);
};

class ARenderer {
  public:
	using Tech = rtech::Tech;
	using Approach = rtech::Approach;
	using Target = rtech::Target;
	using Transition = rtech::Transition;

	struct Attachment {
		LayoutPair layouts;
		vk::Format format;
	};

	ARenderer(not_null<Swapchain*> swapchain, Buffering buffering);
	virtual ~ARenderer() = default;

	static constexpr Extent2D scaleExtent(Extent2D extent, f32 scale) noexcept;
	static RenderImage renderImage(Image const& image) noexcept { return {image.image(), image.view(), cast(image.extent())}; }
	static vk::Viewport viewport(Extent2D extent, ScreenView const& view = {}, glm::vec2 depth = {0.0f, 1.0f}) noexcept;
	static vk::Rect2D scissor(Extent2D extent, ScreenView const& view = {}) noexcept;

	bool hasDepthImage() const noexcept { return m_depthImage.has_value(); }
	std::size_t index() const noexcept { return m_fence.index(); }
	Buffering buffering() const noexcept { return m_fence.buffering(); }

	RenderImage depthImage(Extent2D extent, vk::Format format = {});
	RenderSemaphore makeSemaphore() const;
	vk::RenderPass makeRenderPass(Attachment colour, Attachment depth, vAP<vk::SubpassDependency> deps = {}) const;
	vk::Framebuffer makeFramebuffer(vk::RenderPass renderPass, vAP<vk::ImageView> attachments, Extent2D extent, u32 layers = 1) const;

	virtual Tech tech() const noexcept = 0;

	virtual vk::RenderPass renderPass3D() const noexcept { return m_storage.renderPass; }
	virtual vk::RenderPass renderPassUI() const noexcept { return m_storage.renderPass; }

	virtual std::optional<RenderTarget> beginFrame();
	virtual CommandBuffer beginDraw(RenderTarget const& target, ScreenView const& view, RGBA clear, ClearDepth depth) = 0;
	virtual void endDraw(RenderTarget const&) = 0;
	virtual void endFrame();
	virtual bool submitFrame();

	void refresh() { m_fence.refresh(); }
	void waitForFrame();

	bool canScale() const noexcept;
	f32 renderScale() const noexcept { return m_scale; }
	bool renderScale(f32) noexcept;
	Extent2D renderExtent() const noexcept { return scaleExtent(m_swapchain->display().extent, renderScale()); }

	not_null<Swapchain*> m_swapchain;
	not_null<Device*> m_device;

  protected:
	struct Buf;
	struct Storage {
		RingBuffer<Buf> buf;
		Deferred<vk::RenderPass> renderPass;
	};

	Storage make(Transition transition, TPair<vk::Format> colourDepth = {}) const;
	ktl::expected<Swapchain::Acquire, Swapchain::Flags> acquire(bool begin = true);

	Storage m_storage;
	RenderFence m_fence;
	std::optional<Image> m_depthImage;
	ImageMaker m_imageMaker;

  private:
	std::size_t m_depthIndex = 0;
	f32 m_scale = 1.0f;
};

struct ARenderer::Buf {
	Buf() = default;
	Buf(not_null<Device*> device, vk::CommandPoolCreateFlags flags = {}, QType qtype = QType::eGraphics) {
		pool = makeDeferred<vk::CommandPool>(device, flags, qtype);
		cb = CommandBuffer::make(device, pool, 1).front();
		draw = makeDeferred<vk::Semaphore>(device);
		present = makeDeferred<vk::Semaphore>(device);
	}

	CommandBuffer cb;
	Deferred<vk::CommandPool> pool;
	Deferred<vk::Semaphore> draw;
	Deferred<vk::Semaphore> present;
	Deferred<vk::Framebuffer> framebuffer;
	std::optional<Image> offscreen;
};

constexpr Extent2D ARenderer::scaleExtent(Extent2D extent, f32 scale) noexcept {
	glm::vec2 const ret = glm::vec2(f32(extent.x), f32(extent.y)) * scale;
	return {u32(ret.x), u32(ret.y)};
}
} // namespace le::graphics
