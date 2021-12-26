#pragma once
#include <core/not_null.hpp>
#include <core/span.hpp>
#include <graphics/common.hpp>
#include <graphics/render/framebuffer.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <ktl/fixed_vector.hpp>
#include <optional>

namespace le::graphics {
class VRAM;

enum class VSync : u8 { eOn, eAdaptive, eOff, eCOUNT_ };
constexpr EnumArray<VSync, std::string_view> vSyncNames = {"Vsync On", "Vsync Adaptive", "Vsync Off"};

class Surface {
  public:
	struct Format {
		vk::SurfaceFormatKHR colour;
		vk::Format depth;
		VSync vsync{};
	};

	struct Acquire {
		RenderTarget image;
		std::uint32_t index{};
	};

	struct Sync {
		vk::Semaphore wait;
		vk::Semaphore ssignal;
		vk::Fence fsignal;
	};

	using VSyncs = ktl::enum_flags<VSync, u8>;

	Surface(not_null<VRAM*> vram, Extent2D fbSize = {}, std::optional<VSync> vsync = std::nullopt);
	~Surface();

	static constexpr bool srgb(vk::Format format) noexcept;
	static constexpr bool rgba(vk::Format format) noexcept;
	static constexpr bool bgra(vk::Format format) noexcept;
	static constexpr bool valid(glm::ivec2 framebufferSize) noexcept { return framebufferSize.x > 0 && framebufferSize.y > 0; }

	VRAM& vram() const noexcept { return *m_vram; }
	VSyncs vsyncs() const noexcept { return m_vsyncs; }
	Format const& format() const noexcept { return m_storage.format; }
	Extent2D extent() const noexcept { return cast(m_storage.info.extent); }
	u32 imageCount() const noexcept { return m_storage.info.imageCount; }
	u32 minImageCount() const noexcept { return m_storage.info.minImageCount; }
	BlitFlags blitFlags() const noexcept { return m_storage.blitFlags; }
	vk::ImageUsageFlags usage() const noexcept { return m_createInfo.imageUsage; }

	bool makeSwapchain(Extent2D fbSize = {}, std::optional<VSync> vsync = std::nullopt);

	std::optional<Acquire> acquireNextImage(Extent2D fbSize, vk::Semaphore signal);
	vk::Result submit(Span<vk::CommandBuffer const> cbs, Sync const& sync) const;
	bool present(Extent2D fbSize, Acquire image, vk::Semaphore wait);

  private:
	struct Info {
		vk::Extent2D extent{};
		u32 imageCount{};
		u32 minImageCount{};
	};

	struct Storage {
		vk::UniqueSwapchainKHR swapchain;
		ktl::fixed_vector<vk::UniqueImageView, 8> imageViews;
		ktl::fixed_vector<RenderTarget, 8> images;
		Format format;
		Info info;
		BlitFlags blitFlags;
	};

	Info makeInfo(Extent2D extent) const;
	std::optional<Storage> makeStorage(Storage const& retired, VSync vsync);

	vk::UniqueSurfaceKHR m_surface;
	Storage m_storage;
	Storage m_retired;
	vk::SwapchainCreateInfoKHR m_createInfo;
	VSyncs m_vsyncs;
	not_null<VRAM*> m_vram;
};

// impl

constexpr bool Surface::srgb(vk::Format format) noexcept {
	switch (format) {
	case vk::Format::eR8G8B8Srgb:
	case vk::Format::eB8G8R8Srgb:
	case vk::Format::eR8G8B8A8Srgb:
	case vk::Format::eB8G8R8A8Srgb:
	case vk::Format::eA8B8G8R8SrgbPack32: return true;
	default: return false;
	}
}

constexpr bool Surface::rgba(vk::Format format) noexcept {
	switch (format) {
	case vk::Format::eR8G8B8A8Uint:
	case vk::Format::eR8G8B8A8Unorm:
	case vk::Format::eR8G8B8A8Sint:
	case vk::Format::eR8G8B8A8Snorm:
	case vk::Format::eR8G8B8A8Uscaled:
	case vk::Format::eR8G8B8A8Srgb: return true;
	default: return false;
	}
}

constexpr bool Surface::bgra(vk::Format format) noexcept {
	switch (format) {
	case vk::Format::eB8G8R8A8Uint:
	case vk::Format::eB8G8R8A8Unorm:
	case vk::Format::eB8G8R8A8Sint:
	case vk::Format::eB8G8R8A8Snorm:
	case vk::Format::eB8G8R8A8Uscaled:
	case vk::Format::eB8G8R8A8Srgb: return true;
	default: return false;
	}
}
} // namespace le::graphics
