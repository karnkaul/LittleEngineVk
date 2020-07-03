#pragma once
#include <array>
#include <deque>
#include <filesystem>
#include <functional>
#include <future>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <core/assert.hpp>
#include <core/colour.hpp>
#include <core/log_config.hpp>
#include <core/flags.hpp>
#include <core/std_types.hpp>
#include <engine/window/common.hpp>
#include <engine/gfx/pipeline.hpp>
#include <engine/gfx/shader.hpp>
#include <engine/gfx/texture.hpp>

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
[[maybe_unused]] constexpr std::array g_colourSpaceMap = 
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

	using Flags = TFlags<Flag>;
	using PickDevice = std::function<vk::PhysicalDevice(std::vector<AvailableDevice> const&)>;

	struct
	{
		std::vector<char const*> instanceExtensions;
		CreateSurface createTempSurface;
	} config;

	struct
	{
		PickDevice pickDevice;
		Flags flags;
		log::Level validationLog = log::Level::eWarning;
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
	vk::SharingMode mode;
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

	inline std::vector<vk::ImageView> attachments() const
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

struct ShaderImpl final
{
	static constexpr std::array<vk::ShaderStageFlagBits, size_t(Shader::Type::eCOUNT_)> s_typeToFlagBit = {vk::ShaderStageFlagBits::eVertex,
																										vk::ShaderStageFlagBits::eFragment};

	std::array<vk::ShaderModule, size_t(Shader::Type::eCOUNT_)> shaders;

	inline vk::ShaderModule module(Shader::Type type) const
	{
		ASSERT(shaders.at((size_t)type) != vk::ShaderModule(), "Module not present in Shader!");
		return shaders.at((size_t)type);
	}

	inline std::map<Shader::Type, vk::ShaderModule> modules() const
	{
		std::map<Shader::Type, vk::ShaderModule> ret;
		for (size_t idx = 0; idx < (size_t)Shader::Type::eCOUNT_; ++idx)
		{
			auto const& module = shaders.at(idx);
			if (module != vk::ShaderModule())
			{
				ret[(Shader::Type)idx] = module;
			}
		}
		return ret;
	}
};

struct SamplerImpl final
{
	vk::Sampler sampler;
};

struct TextureImpl final
{
	Image active;
	std::vector<Texture::Raw> raws;
	vk::ImageView imageView;
	vk::Sampler sampler;
	vk::ImageViewType type;
	vk::Format colourSpace;
	std::future<void> copied;
	bool bStbiRaw = false;

#if defined(LEVK_ASSET_HOT_RELOAD)
	Image standby;
	std::vector<stdfs::path> imgIDs;
	FileReader const* pReader = nullptr;
#endif
};

struct MeshImpl final
{
	struct Data
	{
		Buffer buffer;
		std::future<void> copied;
		u32 count = 0;
		void* pMem = nullptr;
	};
	
	Data vbo;
	Data ibo;
};
} // namespace le::gfx
