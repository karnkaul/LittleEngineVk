#pragma once
#include <optional>
#include <core/array_map.hpp>
#include <core/not_null.hpp>
#include <core/span.hpp>
#include <glm/vec2.hpp>
#include <graphics/qflags.hpp>
#include <graphics/render/buffering.hpp>
#include <graphics/render/target.hpp>
#include <graphics/resources.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>
#include <kt/result/result.hpp>

namespace le::graphics {
class VRAM;
class Device;

constexpr ArrayMap<vk::PresentModeKHR, std::string_view, 4> presentModeNames = {{
	{vk::PresentModeKHR::eFifo, "FIFO"},
	{vk::PresentModeKHR::eFifoRelaxed, "FIFO Relaxed"},
	{vk::PresentModeKHR::eImmediate, "Immediate"},
	{vk::PresentModeKHR::eMailbox, "Mailbox"},
}};

class Swapchain {
  public:
	enum class Flag : s8 { ePaused, eOutOfDate, eSuboptimal, eCOUNT_ };
	using Flags = kt::enum_flags<Flag>;

	struct FormatPicker {
		///
		/// \brief Override to provide a custom format
		/// Note: engine assumes sRGB swapchain image format; correct colour in subpasses / shaders if not so
		///
		virtual vk::SurfaceFormatKHR pick(Span<vk::SurfaceFormatKHR const> options) const noexcept;
	};
	struct Display {
		Extent2D extent = {};
		vk::SurfaceTransformFlagBitsKHR transform = {};
	};
	struct CreateInfo {
		u32 imageCount = 2;
		bool vsync = false;
		bool transfer = true;
		FormatPicker const* custom = {};
	};
	struct Acquire {
		RenderImage image;
		u32 index = 0;
	};

	static constexpr bool valid(glm::ivec2 framebufferSize) noexcept;
	static constexpr bool valid(vk::Extent2D extent) noexcept { return valid(glm::ivec2{extent.width, extent.height}); }
	static constexpr bool srgb(vk::Format format) noexcept;

	Swapchain(not_null<VRAM*> vram);
	Swapchain(not_null<VRAM*> vram, CreateInfo const& info, glm::ivec2 framebufferSize = {});
	Swapchain(Swapchain&&);
	Swapchain& operator=(Swapchain&&);
	~Swapchain();

	kt::result<Acquire> acquireNextImage(vk::Semaphore ssignal, vk::Fence fsignal);
	bool present(vk::Semaphore swait);
	bool reconstruct(glm::ivec2 framebufferSize = {}, bool vsync = false);

	bool suboptimal() const noexcept;
	bool paused() const noexcept;

	u8 imageCount() const noexcept { return (u8)m_storage.images.size(); }
	Buffering buffering() const noexcept { return {imageCount()}; }
	Display display() const noexcept { return m_storage.display; }
	Flags flags() const noexcept { return m_storage.flags; }
	vk::SurfaceFormatKHR const& colourFormat() const noexcept { return m_metadata.formats.colour; }
	vk::Format depthFormat() const noexcept { return m_metadata.formats.depth; }
	Span<RenderImage const> images() const noexcept { return m_storage.images; }

	inline static bool s_forceVsync = false;

	not_null<VRAM*> m_vram;
	not_null<Device*> m_device;

  private:
	struct Storage {
		kt::fixed_vector<RenderImage, 4> images;
		vk::SwapchainKHR swapchain;
		std::optional<u32> acquired;

		Display display;
		Flags flags;

		Acquire current() const;
		Acquire current(u32 acquired);
	};
	struct Metadata {
		CreateInfo info;
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
	void destroy(Storage& out_storage);
	void setFlags(vk::Result result);
	void orientCheck();
};

// impl

constexpr bool Swapchain::valid(glm::ivec2 framebufferSize) noexcept { return framebufferSize.x > 0 && framebufferSize.y > 0; }

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
	default: break;
	}
	return false;
}
} // namespace le::graphics
