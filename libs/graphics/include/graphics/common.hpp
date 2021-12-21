#pragma once
#include <core/std_types.hpp>
#include <glm/vec2.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <vulkan/vulkan.hpp>
#include <string_view>
#include <unordered_map>

namespace le::graphics {
enum class BlitFlag { eSrc, eDst, eLinearFilter };
using BlitFlags = ktl::enum_flags<BlitFlag, u8>;

struct BlitCaps {
	BlitFlags optimal;
	BlitFlags linear;
};

struct LayerMip {
	struct Range {
		u32 first = 0U;
		u32 count = 1U;
	};

	Range layer;
	Range mip;

	static LayerMip make(u32 mipCount, u32 firstMip = 0U) noexcept { return LayerMip{{}, {firstMip, mipCount}}; }
};

template <typename T>
using vAP = vk::ArrayProxy<T const> const&;

template <typename T, typename U = T>
using TPair = std::pair<T, U>;

using StagePair = TPair<vk::PipelineStageFlags>;
using AccessPair = TPair<vk::AccessFlags>;
using LayoutPair = TPair<vk::ImageLayout>;
using AspectPair = TPair<vk::ImageAspectFlags>;

using vIL = vk::ImageLayout;
using vIAFB = vk::ImageAspectFlagBits;
using vIUFB = vk::ImageUsageFlagBits;
using vAFB = vk::AccessFlagBits;
using vPSFB = vk::PipelineStageFlagBits;
using ClearDepth = vk::ClearDepthStencilValue;

using Extent2D = glm::tvec2<u32>;
constexpr Extent2D cast(vk::Extent2D extent) noexcept { return {extent.width, extent.height}; }
constexpr Extent2D cast(vk::Extent3D extent) noexcept { return {extent.width, extent.height}; }
constexpr vk::Extent2D cast(Extent2D extent) noexcept { return {extent.x, extent.y}; }

enum class ColourCorrection { eAuto, eNone };

namespace stages {
constexpr vk::ShaderStageFlags v = vk::ShaderStageFlagBits::eVertex;
constexpr vk::ShaderStageFlags f = vk::ShaderStageFlagBits::eFragment;
constexpr vk::ShaderStageFlags vf = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
constexpr vk::ShaderStageFlags c = vk::ShaderStageFlagBits::eCompute;
} // namespace stages

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
} // namespace le::graphics
