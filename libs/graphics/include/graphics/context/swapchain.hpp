#pragma once
#include <optional>
#include <core/not_null.hpp>
#include <core/span.hpp>
#include <glm/vec2.hpp>
#include <graphics/context/render_types.hpp>
#include <graphics/qflags.hpp>
#include <graphics/resources.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>
#include <kt/result/result.hpp>

namespace le::graphics {
class VRAM;
class Device;

class Swapchain {
  public:
	enum class Flag : s8 { ePaused, eOutOfDate, eSuboptimal, eCOUNT_ };
	using Flags = kt::enum_flags<Flag>;

	struct FormatPicker {
		///
		/// \brief Override to provide a custom format
		/// Note: engine assumes sRGB swapchain image format; correct colour in subpasses / shaders if not so
		///
		virtual vk::SurfaceFormatKHR pick(View<vk::SurfaceFormatKHR> options) const noexcept;
	};
	struct Display {
		vk::Extent2D extent = {};
		vk::SurfaceTransformFlagBitsKHR transform = {};
	};
	struct CreateInfo {
		struct {
			u32 imageCount = 2;
			bool vsync = false;
		} desired;

		struct {
			LayoutPair colour = {vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR};
			LayoutPair depth = {vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal};
		} transitions;

		FormatPicker const* custom = {};
	};

	static constexpr std::string_view presentModeName(vk::PresentModeKHR mode) noexcept;
	static constexpr bool valid(glm::ivec2 framebufferSize) noexcept;
	static constexpr bool srgb(vk::Format format) noexcept;

	Swapchain(not_null<VRAM*> vram);
	Swapchain(not_null<VRAM*> vram, CreateInfo const& info, glm::ivec2 framebufferSize = {});
	Swapchain(Swapchain&&);
	Swapchain& operator=(Swapchain&&);
	~Swapchain();

	kt::result<RenderTarget> acquireNextImage(RenderSync const& sync);
	bool present(RenderSync const& sync);
	bool reconstruct(glm::ivec2 framebufferSize = {}, bool vsync = false);

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
	vk::SurfaceFormatKHR const& colourFormat() const noexcept {
		return m_metadata.formats.colour;
	}

	inline static bool s_forceVsync = false;

	not_null<VRAM*> m_vram;
	not_null<Device*> m_device;

  private:
	struct Frame {
		RenderTarget target;
		vk::Fence drawn;
	};
	struct Storage {
		std::optional<Image> depthImage;
		vk::ImageView depthImageView;
		vk::SwapchainKHR swapchain;
		kt::fixed_vector<Frame, 4> frames;
		std::optional<u32> acquired;

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

constexpr std::string_view Swapchain::presentModeName(vk::PresentModeKHR mode) noexcept {
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

constexpr bool Swapchain::valid(glm::ivec2 framebufferSize) noexcept {
	return framebufferSize.x > 0 && framebufferSize.y > 0;
}

constexpr bool Swapchain::srgb(vk::Format format) noexcept {
	switch (format) {
	case vk::Format::eR8G8B8Srgb:
	case vk::Format::eB8G8R8Srgb:
	case vk::Format::eR8G8B8A8Srgb:
	case vk::Format::eB8G8R8A8Srgb:
	case vk::Format::eA8B8G8R8SrgbPack32: {
		return true;
		break;
	}
	default:
		break;
	}
	return false;
}
} // namespace le::graphics
