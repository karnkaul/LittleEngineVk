#pragma once
#include <array>
#include <deque>
#include <functional>
#include <future>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>
#include <vk_mem_alloc.h>
#include <core/colour.hpp>
#include <core/ensure.hpp>
#include <core/std_types.hpp>
#include <dumb_log/log.hpp>
#include <engine/gfx/pipeline.hpp>
#include <engine/levk.hpp>
#include <engine/window/common.hpp>
#include <glm/glm.hpp>
#include <kt/enum_flags/enum_flags.hpp>
#include <vulkan/vulkan.hpp>

#if defined(LEVK_DEBUG)
#if !defined(LEVK_VKRESOURCE_NAMES)
#define LEVK_VKRESOURCE_NAMES
#endif
#endif

namespace le::gfx {
enum class QFlag { eGraphics, ePresent, eTransfer, eCOUNT_ };
using QFlags = kt::enum_flags<QFlag>;

using CreateSurface = std::function<vk::SurfaceKHR(vk::Instance)>;

// clang-format off
[[maybe_unused]] inline constexpr std::array g_colourSpaces = 
{
	vk::Format::eB8G8R8A8Srgb,
	vk::Format::eB8G8R8A8Unorm
};
// clang-format on 


namespace vkFlags
{
inline vk::ShaderStageFlags const vertShader = vk::ShaderStageFlagBits::eVertex;
inline vk::ShaderStageFlags const fragShader = vk::ShaderStageFlagBits::eFragment;
inline vk::ShaderStageFlags const vertFragShader = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
} // namespace vkFlags

struct AvailableDevice final
{
	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties properties;
	vk::PhysicalDeviceFeatures features;
	std::vector<vk::QueueFamilyProperties> queueFamilies;
};

struct InitInfo final
{
	enum class Flag
	{
		eValidation,
		eCOUNT_
	};

	using Flags = kt::enum_flags<Flag>;
	using PickDevice = std::function<vk::PhysicalDevice(std::vector<AvailableDevice> const&)>;

	struct
	{
		CreateSurface createTempSurface;
		Span<char const*> instanceExtensions;
		Span<engine::MemRange> stagingReserve;
	} config;

	struct
	{
		PickDevice pickDevice;
		Flags flags;
		dl::level validationLog = dl::level::warning;
		bool bDedicatedTransfer = true;
	} options;
};

struct ContextInfo final
{
	using GetSize = std::function<glm::ivec2()>;

	struct
	{
		CreateSurface getNewSurface;
		GetSize getFramebufferSize;
		GetSize getWindowSize;
		WindowID window;
	} config;

	struct
	{
		PriorityList<vk::Format> formats;
		PriorityList<vk::ColorSpaceKHR> colourSpaces;
		PriorityList<vk::PresentModeKHR> presentModes;
	} options;
};

struct Queue final
{
	vk::Queue queue;
	u32 familyIndex = 0;
	u32 arrayIndex = 0;
};

struct AllocInfo final
{
	vk::DeviceMemory memory;
	vk::DeviceSize offset = {};
	vk::DeviceSize actualSize = {};
};

struct VkResource
{
#if defined(LEVK_VKRESOURCE_NAMES)
	std::string name;
#endif
	AllocInfo info;
	VmaAllocation handle;
	QFlags queueFlags;
	vk::SharingMode mode;
};

struct Buffer final : VkResource
{
	vk::Buffer buffer;
	vk::DeviceSize writeSize = {};
	void* pMap = nullptr;
};

struct Image final : VkResource
{
	vk::Image image;
	vk::DeviceSize allocatedSize = {};
	vk::Extent3D extent = {};
};

struct LayoutTransition final
{
	vk::ImageLayout pre;
	vk::ImageLayout post;
};

struct ImageViewInfo final
{
	vk::Image image;
	vk::Format format;
	vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor;
	vk::ImageViewType type = vk::ImageViewType::e2D;
};

struct RenderImage final
{
	vk::Image image;
	vk::ImageView view;
};

struct RenderTarget final
{
	RenderImage colour;
	RenderImage depth;
	vk::Extent2D extent;

	inline std::array<vk::ImageView, 2> attachments() const
	{
		return {colour.view, depth.view};
	}
};

inline std::unordered_map<vk::Result, std::string_view> g_vkResultStr = {
	{vk::Result::eErrorOutOfHostMemory, "OutOfHostMemory"},
	{vk::Result::eErrorOutOfDeviceMemory, "OutOfDeviceMemory"},
	{vk::Result::eSuccess, "Success"},
	{vk::Result::eSuboptimalKHR, "SubmoptimalSurface"},
	{vk::Result::eErrorDeviceLost, "DeviceLost"},
	{vk::Result::eErrorSurfaceLostKHR, "SurfaceLost"},
	{vk::Result::eErrorFullScreenExclusiveModeLostEXT, "FullScreenExclusiveModeLost"},
	{vk::Result::eErrorOutOfDateKHR, "OutOfDateSurface"},
};

struct RenderPass final
{
	vk::RenderPass renderPass;
	vk::Format colour;
	vk::Format depth;
};
} // namespace le::gfx
