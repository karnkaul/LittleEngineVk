#include "common.hpp"

namespace le::vuk
{
std::unordered_map<vk::Result, std::string_view> g_vkResultStr = {
	{vk::Result::eErrorOutOfHostMemory, "OutOfHostMemory"},
	{vk::Result::eErrorOutOfDeviceMemory, "OutOfDeviceMemory"},
	{vk::Result::eSuccess, "Success"},
	{vk::Result::eSuboptimalKHR, "SubmoptimalSurface"},
	{vk::Result::eErrorDeviceLost, "DeviceLost"},
	{vk::Result::eErrorSurfaceLostKHR, "SurfaceLost"},
	{vk::Result::eErrorFullScreenExclusiveModeLostEXT, "FullScreenExclusiveModeLost"},
	{vk::Result::eErrorOutOfDateKHR, "OutOfDateSurface"},
};

std::array<f32, 4> const BeginPass::s_black = {0.0f, 0.0f, 0.0f, 1.0f};

ScreenRect::ScreenRect(glm::vec4 const& ltrb) noexcept : left(ltrb.x), top(ltrb.y), right(ltrb.z), bottom(ltrb.w) {}

ScreenRect::ScreenRect(glm::vec2 const& size, glm::vec2 const& leftTop) noexcept
	: left(leftTop.x), top(leftTop.y), right(leftTop.x + size.x), bottom(leftTop.y + size.y)
{
}

f32 ScreenRect::aspect() const
{
	glm::vec2 const size = {right - left, bottom - top};
	return size.x / size.y;
}
} // namespace le::vuk
