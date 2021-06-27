#pragma once
#include <string_view>
#include <unordered_map>
#include <core/lib_logger.hpp>
#include <glm/vec2.hpp>
#include <vulkan/vulkan.hpp>

namespace le::graphics {
using lvl = dl::level;

template <typename T>
using vAP = vk::ArrayProxy<T const> const&;

template <typename T, typename U = T>
using TPair = std::pair<T, U>;

using StagePair = TPair<vk::PipelineStageFlags>;
using AccessPair = TPair<vk::AccessFlags>;
using LayoutPair = TPair<vk::ImageLayout>;

using vIL = vk::ImageLayout;
using vAFB = vk::AccessFlagBits;
using vPSFB = vk::PipelineStageFlagBits;

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
inline LibLogger g_log;

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
