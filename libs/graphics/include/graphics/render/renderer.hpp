#pragma once
#include <graphics/context/command_buffer.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/fence.hpp>

namespace le::graphics {
class Swapchain;

class ARenderer {
  public:
	template <typename T>
	using vAP = vk::ArrayProxy<T const> const&;
	using RenderPasses = kt::fixed_vector<vk::RenderPass, 8>;

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

	struct Command {
		CommandBuffer commandBuffer;
		vk::CommandPool pool;
		vk::CommandBuffer buffer;
	};

	ARenderer(not_null<Swapchain*> swapchain, u8 buffering, vk::Extent2D extent, vk::Format depthFormat);
	virtual ~ARenderer() = default;

	static constexpr vk::Extent2D extent2D(vk::Extent3D extent) { return {extent.width, extent.height}; }
	static RenderImage renderImage(Image const& image) noexcept { return {image.image(), image.view(), extent2D(image.extent())}; }

	bool hasDepthImage() const noexcept { return m_depth.has_value(); }
	std::size_t index() const noexcept { return m_fence.index(); }
	std::size_t buffering() const noexcept { return m_fence.buffering(); }

	std::optional<RenderImage> depthImage(vk::Format depthFormat, vk::Extent2D extent);
	RenderSemaphore makeSemaphore() const;
	Command makeCommand() const;
	vk::RenderPass makeRenderPass(Attachment colour, Attachment depth, vAP<vk::SubpassDependency> deps) const;

	virtual Technique technique() const noexcept = 0;
	virtual RenderPasses renderPasses() const noexcept = 0;

	virtual std::optional<Draw> beginFrame(CommandBuffer::PassInfo const& info) = 0;
	virtual bool endFrame() = 0;

	virtual void refresh() { m_fence.refresh(); }
	virtual void waitForFrame();

	not_null<Swapchain*> m_swapchain;
	not_null<Device*> m_device;

  protected:
	RenderFence m_fence;
	std::optional<Image> m_depth;
	Approach m_tech;
	Target m_target;
};
} // namespace le::graphics
