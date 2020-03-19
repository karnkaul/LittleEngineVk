#pragma once
#include <functional>
#include <optional>
#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "core/flags.hpp"
#include "core/std_types.hpp"
#include "engine/window/common.hpp"

namespace le::vuk
{
using CreateSurface = std::function<vk::SurfaceKHR(vk::Instance)>;

struct AvailableDevice final
{
	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties properties;
	std::vector<vk::QueueFamilyProperties> queueFamilies;
};

struct InitData final
{
	enum class Flag
	{
		eValidation = 0,
		eTest,
		eCOUNT_
	};

	using Flags = TFlags<Flag>;
	using PickDevice = std::function<vk::PhysicalDevice(std::vector<AvailableDevice> const&)>;

	struct
	{
		std::vector<char const*> instanceExtensions;
		CreateSurface createTempSurface;
		u8 graphicsQueueCount = 1;
	} config;

	struct
	{
		PickDevice pickDevice;
		Flags flags;
	} options;
};

struct UBOData final
{
	vk::DeviceSize size;
};

struct ContextData final
{
	using GetFramebufferSize = std::function<glm::ivec2()>;
	struct
	{
		CreateSurface getNewSurface;
		GetFramebufferSize getFramebufferSize;
		WindowID window;
	} config;

	struct
	{
		std::optional<vk::Format> format;
		std::optional<vk::ColorSpaceKHR> colourSpace;
	} options;
};

struct ScreenRect final
{
	f32 left = 0.0f;
	f32 top = 0.0f;
	f32 right = 1.0f;
	f32 bottom = 1.0f;

	constexpr ScreenRect() noexcept = default;
	ScreenRect(glm::vec4 const& ltrb) noexcept;
	explicit ScreenRect(glm::vec2 const& size, glm::vec2 const& leftTop = glm::vec2(0.0f)) noexcept;

	f32 aspect() const;
};

template <typename T, typename = std::enable_if_t<std::is_same_v<T, vk::Buffer> || std::is_same_v<T, vk::Image>>>
struct VkResource final
{
	using vkType = T;

	T resource;
	vk::DeviceMemory memory;
	vk::DeviceSize size = {};
};

namespace ubo
{
struct View final
{
	glm::mat4 mat_vp;
	glm::mat4 mat_v;
};
} // namespace ubo

struct BeginPass final
{
	static const std::array<f32, 4> s_black;

	vk::ClearColorValue colour = vk::ClearColorValue(s_black);
	vk::ClearDepthStencilValue depth = vk::ClearDepthStencilValue(1.0f, 0.0f);

	vk::PipelineLayout pipelineLayout;
};

extern std::unordered_map<vk::Result, std::string_view> g_vkResultStr;
} // namespace le::vuk
