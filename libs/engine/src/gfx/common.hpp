#pragma once
#include <array>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include "core/assert.hpp"
#include "core/colour.hpp"
#include "core/flags.hpp"
#include "core/std_types.hpp"
#include "engine/window/common.hpp"
#include "engine/gfx/pipeline.hpp"
#include "engine/gfx/shader.hpp"
#include "engine/gfx/texture.hpp"

#if defined(LEVK_DEBUG)
#if !defined(LEVK_VKRESOURCE_NAMES)
#define LEVK_VKRESOURCE_NAMES
#endif
#endif

namespace stdfs = std::filesystem;

namespace le::gfx
{
enum class QFlag
{
	eGraphics,
	ePresent,
	eTransfer,
	eCOUNT_
};
using QFlags = TFlags<QFlag>;

using CreateSurface = std::function<vk::SurfaceKHR(vk::Instance)>;

// clang-format off
constexpr std::array g_colourSpaceMap = 
{
	vk::Format::eB8G8R8A8Srgb,
	vk::Format::eB8G8R8A8Unorm
};

constexpr std::array g_presentModeMap = 
{
	vk::PresentModeKHR::eFifo,
	vk::PresentModeKHR::eMailbox,
	vk::PresentModeKHR::eImmediate,
};

constexpr std::array g_polygonModeMap = 
{
	vk::PolygonMode::eFill,
	vk::PolygonMode::eLine
};

constexpr std::array g_cullModeMap = 
{
	vk::CullModeFlagBits::eNone,
	vk::CullModeFlagBits::eBack,
	vk::CullModeFlagBits::eFront
};

constexpr std::array g_frontFaceMap = 
{
	vk::FrontFace::eCounterClockwise,
	vk::FrontFace::eClockwise
};
// clang-format on 


namespace vkFlags
{
inline vk::ShaderStageFlags const vertShader = vk::ShaderStageFlagBits::eVertex;
inline vk::ShaderStageFlags const fragShader = vk::ShaderStageFlagBits::eVertex;
inline vk::ShaderStageFlags const vertFragShader = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
} // namespace vkFlags

struct AvailableDevice final
{
	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties properties;
	std::vector<vk::QueueFamilyProperties> queueFamilies;
};

struct InitInfo final
{
	enum class Flag
	{
		eValidation,
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

struct PresenterInfo final
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

struct UniqueQueues final
{
	vk::SharingMode mode;
	std::vector<u32> indices;
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
};

struct Buffer final : VkResource
{
	vk::Buffer buffer;
	vk::DeviceSize writeSize = {};
};

struct Image final : VkResource
{
	vk::Image image;
	vk::DeviceSize allocatedSize = {};
	vk::Extent3D extent = {};
};

extern std::unordered_map<vk::Result, std::string_view> g_vkResultStr;

struct ShaderImpl final
{
	static std::array<vk::ShaderStageFlagBits, size_t(Shader::Type::eCOUNT_)> const s_typeToFlagBit;

	std::array<vk::ShaderModule, size_t(Shader::Type::eCOUNT_)> shaders;

	vk::ShaderModule module(Shader::Type type) const;
	std::map<Shader::Type, vk::ShaderModule> modules() const;
};

struct SamplerImpl final
{
	vk::Sampler sampler;
};

struct TextureImpl final
{
	Image active;
	Texture::Raw raw;
	vk::ImageView imageView;
	vk::Fence loaded;
	bool bStbiRaw = false;

#if defined(LEVK_ASSET_HOT_RELOAD)
	Image standby;
	stdfs::path imgID;
	FileReader const* pReader = nullptr;
	bool bReloading = false;
#endif
};

struct MeshImpl final
{
	Buffer vbo;
	Buffer ibo;
	vk::Fence vboCopied;
	vk::Fence iboCopied;
	u32 vertexCount = 0;
	u32 indexCount = 0;
};

namespace vbo
{
constexpr u32 vertexBinding = 0;

vk::VertexInputBindingDescription bindingDescription();
std::vector<vk::VertexInputAttributeDescription> attributeDescriptions();
} // namespace vbo
} // namespace le::gfx
