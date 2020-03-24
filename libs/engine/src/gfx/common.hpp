#pragma once
#include <filesystem>
#include <functional>
#include <optional>
#include <set>
#include <unordered_map>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "core/colour.hpp"
#include "core/flags.hpp"
#include "core/std_types.hpp"
#include "engine/window/common.hpp"

namespace stdfs = std::filesystem;

namespace le::gfx
{
enum class QFlag
{
	eGraphics = 0,
	ePresent,
	eTransfer,
	eCOUNT_
};
using QFlags = TFlags<QFlag>;

enum class ShaderType : u8
{
	eVertex = 0,
	eFragment,
	eCOUNT_
};

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

struct PresenterData final
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

struct AllocInfo final
{
	vk::DeviceMemory memory;
	vk::DeviceSize offset = {};
	vk::DeviceSize actualSize = {};
};

struct Resource
{
	AllocInfo info;
	VmaAllocation handle;
};

struct Buffer final : Resource
{
	vk::Buffer buffer;
	vk::DeviceSize writeSize = {};
};

struct Image final : Resource
{
	vk::Image image;
};

struct ClearValues final
{
	glm::vec2 depthStencil = {1.0f, 0.0f};
	Colour colour = colours::Black;
};

struct ShaderData final
{
	std::string id;
	std::unordered_map<ShaderType, bytearray> codeMap;
	std::unordered_map<ShaderType, stdfs::path> codeIDMap;
	class IOReader const* pReader = nullptr;
};

struct PipelineData final
{
	vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
	vk::CullModeFlagBits cullMode = vk::CullModeFlagBits::eNone;
	vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
	vk::ColorComponentFlags colourWriteMask =
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	std::string name;
	std::set<vk::DynamicState> dynamicStates;
	std::vector<vk::DescriptorSetLayout> setLayouts;
	f32 staticLineWidth = 1.0f;
	class Shader* pShader = nullptr;
	bool bBlend = false;

	// TODO: REMOVE
	vk::RenderPass renderPass;
};

extern std::unordered_map<vk::Result, std::string_view> g_vkResultStr;
} // namespace le::gfx
