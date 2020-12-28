#pragma once
#include <optional>
#include <core/ref.hpp>
#include <core/span.hpp>
#include <glm/vec2.hpp>
#include <graphics/context/render_types.hpp>
#include <graphics/qflags.hpp>
#include <graphics/resources.hpp>

namespace le::graphics {
class VRAM;
class Device;

class Swapchain {
  public:
	enum class Flag : s8 { ePaused, eOutOfDate, eSuboptimal, eCOUNT_ };
	using Flags = kt::enum_flags<Flag>;

	struct Display {
		vk::Extent2D extent = {};
		vk::SurfaceTransformFlagBitsKHR transform = {};
	};
	struct CreateInfo {
		using vF = vk::Format;
		static constexpr auto defaultColourSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
		static constexpr std::array defaultColourFormats = {vF::eB8G8R8A8Srgb, vF::eR8G8B8A8Srgb};
		static constexpr std::array defaultDepthFormats = {vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint};
		static constexpr auto defaultPresentMode = vk::PresentModeKHR::eFifo;

		struct {
			Span<vk::ColorSpaceKHR> colourSpaces = defaultColourSpace;
			Span<vk::Format> colourFormats = defaultColourFormats;
			Span<vk::Format> depthFormats = defaultDepthFormats;
			Span<vk::PresentModeKHR> presentModes = defaultPresentMode;
			u32 imageCount = 2;
		} desired;

		struct {
			LayoutPair colour = {vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR};
			LayoutPair depth = {vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal};
		} transitions;
	};

	static constexpr std::string_view presentModeName(vk::PresentModeKHR mode) noexcept;
	static constexpr bool valid(glm::ivec2 framebufferSize) noexcept;

	Swapchain(VRAM& vram);
	Swapchain(VRAM& vram, CreateInfo const& info, glm::ivec2 framebufferSize = {});
	Swapchain(Swapchain&&);
	Swapchain& operator=(Swapchain&&);
	~Swapchain();

	std::optional<RenderTarget> acquireNextImage(RenderSync const& sync);
	bool present(RenderSync const& sync);
	bool reconstruct(glm::ivec2 framebufferSize = {}, Span<vk::PresentModeKHR> desiredModes = {});

	bool suboptimal() const noexcept;
	bool paused() const noexcept;

	Display display() const noexcept {
		return m_storage.current;
	}
	Flags flags() const noexcept {
		return m_storage.flags;
	}
	vk::RenderPass renderPass() const noexcept {
		return m_metadata.renderPass;
	}
	vk::SurfaceFormatKHR colourFormat() const noexcept {
		return m_metadata.formats.colour;
	}

	Ref<VRAM> m_vram;
	Ref<Device> m_device;

  private:
	struct Frame {
		RenderTarget target;
		vk::Fence drawn;
	};
	struct Storage {
		std::optional<Image> depthImage;
		vk::ImageView depthImageView;
		vk::SwapchainKHR swapchain;
		std::vector<Frame> frames;
		std::optional<vk::ResultValue<u32>> acquired;

		Display current;
		u8 imageCount = 0;
		Flags flags;

		Frame& frame();
	};
	struct Metadata {
		CreateInfo info;
		vk::RenderPass renderPass;
		vk::SurfaceKHR surface;
		vk::SwapchainKHR retired;
		vk::PresentModeKHR presentMode;
		std::optional<Display> original;
		std::vector<vk::PresentModeKHR> availableModes;
		struct {
			vk::SurfaceFormatKHR colour;
			vk::Format depth;
		} formats;
	};

	Storage m_storage;
	Metadata m_metadata;

  private:
	bool construct(glm::ivec2 framebufferSize);
	void makeRenderPass();
	void destroy(Storage& out_storage, bool bMeta);
	void setFlags(vk::Result result);
	void orientCheck();
};

// impl

inline constexpr std::string_view Swapchain::presentModeName(vk::PresentModeKHR mode) noexcept {
	switch (mode) {
	case vk::PresentModeKHR::eFifo:
		return "FIFO";
	case vk::PresentModeKHR::eFifoRelaxed:
		return "FIFO Relaxed";
	case vk::PresentModeKHR::eImmediate:
		return "Immediate";
	case vk::PresentModeKHR::eMailbox:
		return "Mailbox";
	default:
		return "Other";
	}
}

inline constexpr bool Swapchain::valid(glm::ivec2 framebufferSize) noexcept {
	return framebufferSize.x > 0 && framebufferSize.y > 0;
}
} // namespace le::graphics
