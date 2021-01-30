#pragma once
#include <functional>
#include <vector>
#include <engine/window/common.hpp>
#include <gfx/common.hpp>
#include <glm/glm.hpp>
#include <kt/enum_flags/enum_flags.hpp>
#include <kt/result/result.hpp>
#include <vulkan/vulkan.hpp>

namespace le::gfx {
struct RenderFrame final {
	RenderTarget swapchain;
	vk::Fence drawn;
};

class RenderContext final {
  public:
	enum class Flag : s8 { eRenderPaused, eOutOfDate, eCOUNT_ };
	using Flags = kt::enum_flags<Flag>;
	template <typename T>
	using Result = kt::result_t<T>;

  private:
	struct Metadata final {
		ContextInfo info;
		vk::SurfaceKHR surface;

		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> colourFormats;
		std::vector<vk::PresentModeKHR> presentModes;

		vk::SurfaceFormatKHR colourFormat;
		vk::Format depthFormat;
		vk::PresentModeKHR presentMode;

		void refresh();
		bool ready() const;
		vk::SurfaceFormatKHR bestColourFormat() const;
		vk::Format bestDepthFormat() const;
		vk::PresentModeKHR bestPresentMode() const;
		vk::Extent2D extent(glm::ivec2 const& windowSize) const;
	};

	struct Swapchain final {
		Image depthImage;
		vk::ImageView depthImageView;
		vk::SwapchainKHR swapchain;
		std::vector<RenderFrame> frames;

		vk::Extent2D extent;
		u32 imageIndex = 0;
		u8 imageCount = 0;

		RenderFrame& frame();
	};

  public:
	static std::string const s_tName;
	std::string m_name;

  public:
	Swapchain m_swapchain;
	vk::SwapchainKHR m_retiring;
	Flags m_flags;
	Metadata m_metadata;

  private:
	WindowID m_window;

  public:
	RenderContext(ContextInfo const& info);
	~RenderContext();

  public:
	void onFramebufferResize();

	Result<RenderTarget> acquireNextImage(vk::Semaphore setDrawReady, vk::Fence setOnDrawn);
	bool present(vk::Semaphore wait);

	vk::Format colourFormat() const;
	vk::Format depthFormat() const;
	glm::vec2 framebufferSize() const;

	bool recreateSwapchain();

  private:
	bool createSwapchain();
	void destroySwapchain();
	void cleanup();
};
} // namespace le::gfx
