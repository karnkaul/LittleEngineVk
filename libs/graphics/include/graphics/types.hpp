#pragma once
#include <string_view>
#include <unordered_map>
#include <vk_mem_alloc.h>
#include <core/ref.hpp>
#include <core/span.hpp>
#include <core/std_types.hpp>
#include <kt/enum_flags/enum_flags.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
#if defined(LEVK_DEBUG)
#if !defined(LEVK_VKRESOURCE_NAMES)
#define LEVK_VKRESOURCE_NAMES
#endif
#endif

enum class QType { eGraphics, ePresent, eTransfer, eCOUNT_ };
using QFlags = kt::enum_flags<QType>;

constexpr std::string_view g_name = "Graphics";

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

constexpr vk::DeviceSize operator""_MB(unsigned long long size) {
	return size << 20;
}

template <typename T>
using vAP = vk::ArrayProxy<T const> const&;

namespace stages {
constexpr vk::ShaderStageFlags v = vk::ShaderStageFlagBits::eVertex;
constexpr vk::ShaderStageFlags f = vk::ShaderStageFlagBits::eFragment;
constexpr vk::ShaderStageFlags vf = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
constexpr vk::ShaderStageFlags c = vk::ShaderStageFlagBits::eCompute;
} // namespace stages

struct AvailableDevice final {
	inline std::string_view name() const noexcept {
		return std::string_view(properties.deviceName);
	}

	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties properties;
	vk::PhysicalDeviceFeatures features;
	std::vector<vk::QueueFamilyProperties> queueFamilies;
};

struct RenderImage final {
	vk::Image image;
	vk::ImageView view;
	vk::Extent2D extent;
};

struct RenderTarget final {
	RenderImage colour;
	RenderImage depth;
	vk::Extent2D extent;

	inline std::array<vk::ImageView, 2> attachments() const {
		return {colour.view, depth.view};
	}
};

struct AllocInfo final {
	vk::DeviceMemory memory;
	vk::DeviceSize offset = {};
	vk::DeviceSize actualSize = {};
};

struct VkResource {
#if defined(LEVK_VKRESOURCE_NAMES)
	std::string name;
#endif
	AllocInfo info;
	VmaAllocation handle;
	QFlags queueFlags;
	vk::SharingMode mode;
	u64 guid = 0;
};

struct Buffer final : VkResource {
	enum class Type { eCpuToGpu, eGpuOnly };
	struct CreateInfo;
	struct Span;

	vk::Buffer buffer;
	vk::DeviceSize writeSize = {};
	vk::BufferUsageFlags usage;
	Type type;
	void* pMap = nullptr;
};

struct Buffer::Span {
	std::size_t offset = 0;
	std::size_t size = 0;
};

struct Image final : VkResource {
	struct CreateInfo;

	vk::Image image;
	vk::DeviceSize allocatedSize = {};
	vk::Extent3D extent = {};
};

struct RawImage {
	Span<u8> bytes;
	int width = 0;
	int height = 0;
};

struct VertexInputInfo {
	std::vector<vk::VertexInputBindingDescription> bindings;
	std::vector<vk::VertexInputAttributeDescription> attributes;
};

template <typename T>
struct VertexInfoFactory {
	VertexInputInfo operator()(u32 binding) const;
};

template <typename T>
constexpr bool default_v(T const& t) noexcept {
	return t == T();
}
} // namespace le::graphics
