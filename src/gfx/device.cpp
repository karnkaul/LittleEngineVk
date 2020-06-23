#include <algorithm>
#include <memory>
#include <set>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/utils.hpp>
#include <window/window_impl.hpp>
#include <gfx/deferred.hpp>
#include <gfx/device.hpp>
#include <gfx/vram.hpp>
#include <gfx/resource_descriptors.hpp>

namespace le::gfx
{
namespace
{
static std::string const s_tInstance = utils::tName<vk::Instance>();
static std::string const s_tDevice = utils::tName<vk::Device>();

struct QueueFamily final
{
	u32 index = 0;
	u32 queueCount = 0;
	QFlags flags;
};

vk::DispatchLoaderDynamic g_loader;
vk::DebugUtilsMessengerEXT g_debugMessenger;

#define VK_LOG_MSG pCallbackData && pCallbackData->pMessage ? pCallbackData->pMessage : "UNKNOWN"

VKAPI_ATTR vk::Bool32 VKAPI_CALL validationCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
													VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void*)
{
	static std::string_view const name = "vk::validation";
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		LOG_E("[{}] {}", name, VK_LOG_MSG);
		ASSERT(false, VK_LOG_MSG);
		return true;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		if ((u8)g_instance.validationLog <= (u8)log::Level::eWarning)
		{
			LOG_W("[{}] {}", name, VK_LOG_MSG);
		}
		break;
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		if ((u8)g_instance.validationLog <= (u8)log::Level::eInfo)
		{
			LOG_I("[{}] {}", name, VK_LOG_MSG);
		}
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		if ((u8)g_instance.validationLog <= (u8)log::Level::eDebug)
		{
			LOG_D("[{}] {}", name, VK_LOG_MSG);
		}
		break;
	}
	return false;
}

bool initDevice2(vk::Instance vkInst, std::vector<char const*> const& layers, InitInfo const& initInfo)
{
	// Instance
	std::vector<char const*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME};
	std::string deviceName = "UNKNOWN";
	Instance instance;
	vk::SurfaceKHR surface;
	try
	{
		surface = initInfo.config.createTempSurface(vkInst);
		auto physicalDevices = vkInst.enumeratePhysicalDevices();
		std::vector<AvailableDevice> availableDevices;
		availableDevices.reserve(physicalDevices.size());
		for (auto const& physicalDevice : physicalDevices)
		{
			std::set<std::string_view> missingExtensions(deviceExtensions.begin(), deviceExtensions.end());
			auto const extensions = physicalDevice.enumerateDeviceExtensionProperties();
			for (size_t idx = 0; idx < extensions.size() && !missingExtensions.empty(); ++idx)
			{
				missingExtensions.erase(std::string_view(extensions.at(idx).extensionName));
			}
			for (auto extension : missingExtensions)
			{
				LOG_E("[{}] Required extensions not present on physical device [{}]!", s_tDevice, extension);
			}
			ASSERT(missingExtensions.empty(), "Required extension not present!");
			if (missingExtensions.empty())
			{
				AvailableDevice availableDevice;
				availableDevice.properties = physicalDevice.getProperties();
				availableDevice.queueFamilies = physicalDevice.getQueueFamilyProperties();
				availableDevice.features = physicalDevice.getFeatures();
				availableDevice.physicalDevice = physicalDevice;
				if (availableDevice.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
				{
					instance.physicalDevice = physicalDevice;
				}
				availableDevices.push_back(std::move(availableDevice));
			}
		}
		if (initInfo.options.pickDevice)
		{
			instance.physicalDevice = initInfo.options.pickDevice(availableDevices);
		}
		if (instance.physicalDevice == vk::PhysicalDevice() && !availableDevices.empty())
		{
			instance.physicalDevice = availableDevices.front().physicalDevice;
		}
		if (instance.physicalDevice == vk::PhysicalDevice())
		{
			if (surface != vk::SurfaceKHR())
			{
				instance.destroy(surface);
			}
			throw std::runtime_error("Failed to select a physical device!");
		}
		auto const properties = instance.physicalDevice.getProperties();
		deviceName = properties.deviceName;
		instance.deviceLimits = properties.limits;
		instance.lineWidthMin = properties.limits.lineWidthRange[0];
		instance.lineWidthMax = properties.limits.lineWidthRange[1];
		rd::ImageSamplers::clampDiffSpecCount(instance.deviceLimits.maxPerStageDescriptorSamplers);
	}
	catch (std::exception const&)
	{
		if (surface != vk::SurfaceKHR())
		{
			instance.destroy(surface);
		}
		// TODO: MOVE
		if (g_debugMessenger != vk::DebugUtilsMessengerEXT())
		{
			vkInst.destroy(g_debugMessenger, nullptr, g_loader);
		}
		vkInst.destroy();
		return false;
	}
	instance.instance = vkInst;

	// Device
	Device device;
	try
	{
		auto const queueFamilyProperties = instance.physicalDevice.getQueueFamilyProperties();
		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		std::unordered_map<u32, QueueFamily> queueFamilies;
		QFlags found;
		size_t graphicsFamilyIdx = 0;
		for (size_t idx = 0; idx < queueFamilyProperties.size() && !found.bits.all(); ++idx)
		{
			auto const& queueFamily = queueFamilyProperties.at(idx);
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
			{
				if (!found.isSet(QFlag::eGraphics))
				{
					QueueFamily& family = queueFamilies[(u32)idx];
					family.index = (u32)idx;
					family.queueCount = queueFamily.queueCount;
					family.flags |= QFlag::eGraphics;
					found.set(QFlag::eGraphics);
					graphicsFamilyIdx = idx;
				}
			}
			if (instance.physicalDevice.getSurfaceSupportKHR((u32)idx, surface))
			{
				if (!found.isSet(QFlag::ePresent))
				{
					QueueFamily& family = queueFamilies[(u32)idx];
					family.index = (u32)idx;
					family.queueCount = queueFamily.queueCount;
					family.flags |= QFlag::ePresent;
					found.set(QFlag::ePresent);
				}
			}
			if (initInfo.options.bDedicatedTransfer && queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)
			{
				if (found.isSet(QFlag::eGraphics) && idx != graphicsFamilyIdx && !found.isSet(QFlag::eTransfer))
				{
					QueueFamily& family = queueFamilies[(u32)idx];
					family.index = (u32)idx;
					family.queueCount = queueFamily.queueCount;
					family.flags |= QFlag::eTransfer;
					found.set(QFlag::eTransfer);
				}
			}
		}
		if (!found.isSet(QFlag::eGraphics) || !found.isSet(QFlag::ePresent))
		{
			throw std::runtime_error("Failed to obtain graphics/present queues from device!");
		}
		f32 priority = 1.0f;
		f32 const priorities[] = {0.7f, 0.3f};
		for (auto& [index, queueFamily] : queueFamilies)
		{
			vk::DeviceQueueCreateInfo queueCreateInfo;
			queueCreateInfo.queueFamilyIndex = index;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &priority;
			if (queueFamily.flags.isSet(QFlag::eGraphics))
			{
				device.queues.graphics.familyIndex = index;
			}
			if (queueFamily.flags.isSet(QFlag::ePresent))
			{
				device.queues.present.familyIndex = index;
			}
			if (queueFamily.flags.isSet(QFlag::eTransfer))
			{
				device.queues.transfer.familyIndex = index;
			}
			if (initInfo.options.bDedicatedTransfer && !found.isSet(QFlag::eTransfer) && queueFamily.flags.isSet(QFlag::eGraphics)
				&& queueFamily.queueCount > 1)
			{
				queueCreateInfo.queueCount = 2;
				queueCreateInfo.pQueuePriorities = priorities;
				device.queues.transfer.arrayIndex = 1;
				found.set(QFlag::eTransfer);
			}
			queueCreateInfos.push_back(std::move(queueCreateInfo));
		}
		if (!found.isSet(QFlag::eTransfer))
		{
			device.queues.transfer = device.queues.graphics;
		}
		vk::PhysicalDeviceFeatures deviceFeatures;
		deviceFeatures.fillModeNonSolid = true;
		deviceFeatures.wideLines = instance.lineWidthMax > 1.0f;
		vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures;
		descriptorIndexingFeatures.runtimeDescriptorArray = true;
		vk::DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = (u32)queueCreateInfos.size();
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		deviceCreateInfo.pNext = &descriptorIndexingFeatures;
		if (!layers.empty())
		{
			deviceCreateInfo.enabledLayerCount = (u32)layers.size();
			deviceCreateInfo.ppEnabledLayerNames = layers.data();
		}
		deviceCreateInfo.enabledExtensionCount = (u32)deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.empty() ? nullptr : deviceExtensions.data();
		device.device = instance.physicalDevice.createDevice(deviceCreateInfo);
		device.queues.graphics.queue = device.device.getQueue(device.queues.graphics.familyIndex, device.queues.graphics.arrayIndex);
		device.queues.present.queue = device.device.getQueue(device.queues.present.familyIndex, device.queues.present.arrayIndex);
		device.queues.transfer.queue = device.device.getQueue(device.queues.transfer.familyIndex, device.queues.transfer.arrayIndex);
		instance.instance.destroy(surface);
	}
	catch (std::exception const&)
	{
		if (device.device != vk::Device())
		{
			device.device.destroy();
		}
		if (surface != vk::SurfaceKHR())
		{
			instance.instance.destroy(surface);
		}
		if (g_debugMessenger != vk::DebugUtilsMessengerEXT())
		{
			instance.instance.destroy(g_debugMessenger, nullptr, g_loader);
		}
		instance.instance.destroy();
		return false;
	}
	g_instance = instance;
	g_device = device;
	LOG_I("[{}] constructed. Using GPU: [{}]", s_tDevice, deviceName);
	return true;
}

void init(InitInfo const& initInfo)
{
	std::vector<char const*> requiredLayers;
	std::set<char const*> requiredExtensionsSet = {initInfo.config.instanceExtensions.begin(), initInfo.config.instanceExtensions.end()};
	g_instance.validationLog = initInfo.options.validationLog;
	if (initInfo.options.flags.isSet(InitInfo::Flag::eValidation))
	{
		requiredExtensionsSet.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
	}
	std::vector<char const*> const requiredExtensions = {requiredExtensionsSet.begin(), requiredExtensionsSet.end()};
	auto const layers = vk::enumerateInstanceLayerProperties();
	for (auto szRequiredLayer : requiredLayers)
	{
		bool bFound = false;
		std::string_view const requiredLayer(szRequiredLayer);
		for (auto const& layer : layers)
		{
			if (requiredLayer == std::string_view(layer.layerName))
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			LOG_E("[{}] Required layer [{}] not available!", s_tInstance, szRequiredLayer);
			throw std::runtime_error("Failed to create Instance!");
		}
	}
	vk::Instance vkInst;
	{
		vk::ApplicationInfo appInfo;
		appInfo.pApplicationName = "LittleEngineVk Game";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "LittleEngineVk";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		vk::InstanceCreateInfo createInfo;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.ppEnabledExtensionNames = requiredExtensions.empty() ? nullptr : requiredExtensions.data();
		createInfo.enabledExtensionCount = (u32)requiredExtensions.size();
		createInfo.ppEnabledLayerNames = requiredLayers.empty() ? nullptr : requiredLayers.data();
		createInfo.enabledLayerCount = (u32)requiredLayers.size();
		vkInst = vk::createInstance(createInfo, nullptr);
	}
	vk::DynamicLoader dl;
	g_loader.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	g_loader.init(vkInst);
	if (initInfo.options.flags.isSet(InitInfo::Flag::eValidation))
	{
		vk::DebugUtilsMessengerCreateInfoEXT createInfo;
		using vksev = vk::DebugUtilsMessageSeverityFlagBitsEXT;
		createInfo.messageSeverity = vksev::eError | vksev::eWarning | vksev::eInfo | vksev::eVerbose;
		using vktype = vk::DebugUtilsMessageTypeFlagBitsEXT;
		createInfo.messageType = vktype::eGeneral | vktype::ePerformance | vktype::eValidation;
		createInfo.pfnUserCallback = &validationCallback;
		try
		{
			g_debugMessenger = vkInst.createDebugUtilsMessengerEXT(createInfo, nullptr, g_loader);
		}
		catch (std::exception const& e)
		{
			vkInst.destroy();
			throw std::runtime_error(e.what());
		}
	}
	if (!initDevice2(vkInst, requiredLayers, initInfo))
	{
		throw std::runtime_error("Failed to initialise Device!");
	}

	vram::init();
	rd::init();
	LOG_I("[{}] and [{}] successfully initialised", s_tInstance, s_tDevice);
}

void deinit()
{
	deferred::deinit();
	rd::deinit();
	vram::deinit();
	if (g_device.device != vk::Device())
	{
		g_device.device.destroy();
	}
	if (g_instance.instance != vk::Instance())
	{
		if (g_debugMessenger != vk::DebugUtilsMessengerEXT())
		{
			g_instance.instance.destroy(g_debugMessenger, nullptr, g_loader);
		}
		g_instance.instance.destroy();
	}
	g_instance = {};
	g_device = {};
	LOG_I("[{}] and [{}] deinitialised", s_tInstance, s_tDevice);
	return;
}
} // namespace

u32 Instance::findMemoryType(u32 typeFilter, vk::MemoryPropertyFlags properties) const
{
	auto const memProperties = physicalDevice.getMemoryProperties();
	for (u32 i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("Failed to find suitable memory type!");
}

f32 Instance::lineWidth(f32 desired) const
{
	return std::clamp(desired, lineWidthMin, lineWidthMax);
}

TResult<vk::Format> Instance::supportedFormat(PriorityList<vk::Format> const& desired, vk::ImageTiling tiling,
											  vk::FormatFeatureFlags features)
{
	for (auto format : desired)
	{
		vk::FormatProperties props = physicalDevice.getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}
	return {};
}

bool Device::isValid(vk::SurfaceKHR surface) const
{
	if (g_instance.physicalDevice != vk::PhysicalDevice())
	{
		return g_instance.physicalDevice.getSurfaceSupportKHR(queues.present.familyIndex, surface);
	}
	return false;
}

UniqueQueues Device::uniqueQueues(QFlags flags) const
{
	UniqueQueues ret;
	ret.indices.reserve(3);
	if (flags.isSet(QFlag::eGraphics))
	{
		ret.indices.push_back(queues.graphics.familyIndex);
	}
	if (flags.isSet(QFlag::ePresent) && queues.graphics.familyIndex != queues.present.familyIndex)
	{
		ret.indices.push_back(queues.present.familyIndex);
	}
	if (flags.isSet(QFlag::eTransfer) && queues.transfer.familyIndex != queues.graphics.familyIndex)
	{
		ret.indices.push_back(queues.transfer.familyIndex);
	}
	ret.mode = ret.indices.size() > 1 ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
	return ret;
}

void Device::waitIdle() const
{
	device.waitIdle();
}

vk::Fence Device::createFence(bool bSignalled) const
{
	vk::FenceCreateFlags flags = bSignalled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlags();
	return g_device.device.createFence(flags);
}

void Device::waitFor(vk::Fence optional) const
{
	if (optional != vk::Fence())
	{
#if defined(LEVK_DEBUG)
		constexpr static u64 s_wait = 1000ULL * 1000 * 5000;
		auto const result = g_device.device.waitForFences(optional, true, s_wait);
		ASSERT(result != vk::Result::eTimeout && result != vk::Result::eErrorDeviceLost, "Fence wait failure!");
		if (result == vk::Result::eTimeout || result == vk::Result::eErrorDeviceLost)
		{
			LOG_E("[{}] Fence wait failure!", s_tDevice);
		}
#else
		g_device.device.waitForFences(optional, true, maxVal<u64>());
#endif
	}
}

void Device::waitAll(vk::ArrayProxy<const vk::Fence> validFences) const
{
	if (!validFences.empty())
	{
#if defined(LEVK_DEBUG)
		constexpr static u64 s_wait = 1000ULL * 1000 * 5000;
		auto const result = g_device.device.waitForFences(std::move(validFences), true, s_wait);
		ASSERT(result != vk::Result::eTimeout && result != vk::Result::eErrorDeviceLost, "Fence wait failure!");
		if (result == vk::Result::eTimeout || result == vk::Result::eErrorDeviceLost)
		{
			LOG_E("[{}] Fence wait failure!", s_tDevice);
		}
#else
		g_device.device.waitForFences(std::move(validFences), true, maxVal<u64>());
#endif
	}
}

void Device::resetFence(vk::Fence optional) const
{
	if (optional != vk::Fence())
	{
		g_device.device.resetFences(optional);
	}
}

void Device::resetAll(vk::ArrayProxy<const vk::Fence> validFences) const
{
	if (!validFences.empty())
	{
		g_device.device.resetFences(std::move(validFences));
	}
}

bool Device::isSignalled(vk::Fence fence) const
{
	if (fence != vk::Fence())
	{
		return g_device.device.getFenceStatus(fence) == vk::Result::eSuccess;
	}
	return true;
}

bool Device::allSignalled(Span<vk::Fence const> fences) const
{
	return std::all_of(fences.begin(), fences.end(), [this](auto fence) { return isSignalled(fence); });
}

vk::ImageView Device::createImageView(ImageViewInfo const& info)
{
	vk::ImageViewCreateInfo createInfo;
	createInfo.image = info.image;
	createInfo.viewType = info.type;
	createInfo.format = info.format;
	createInfo.components.r = createInfo.components.g = createInfo.components.b = createInfo.components.a = vk::ComponentSwizzle::eIdentity;
	createInfo.subresourceRange.aspectMask = info.aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = info.type == vk::ImageViewType::eCube ? 6 : 1;
	return device.createImageView(createInfo);
}

vk::PipelineLayout Device::createPipelineLayout(vk::ArrayProxy<vk::PushConstantRange const> pushConstants,
												vk::ArrayProxy<vk::DescriptorSetLayout const> setLayouts)
{
	vk::PipelineLayoutCreateInfo createInfo;
	createInfo.setLayoutCount = setLayouts.size();
	createInfo.pSetLayouts = setLayouts.data();
	createInfo.pushConstantRangeCount = pushConstants.size();
	createInfo.pPushConstantRanges = pushConstants.data();
	return device.createPipelineLayout(createInfo);
}

vk::DescriptorSetLayout Device::createDescriptorSetLayout(vk::ArrayProxy<vk::DescriptorSetLayoutBinding const> bindings)
{
	vk::DescriptorSetLayoutCreateInfo createInfo;
	createInfo.bindingCount = bindings.size();
	createInfo.pBindings = bindings.data();
	return device.createDescriptorSetLayout(createInfo);
}

vk::DescriptorPool Device::createDescriptorPool(vk::ArrayProxy<vk::DescriptorPoolSize const> poolSizes, u32 maxSets)
{
	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.poolSizeCount = poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = maxSets;
	return device.createDescriptorPool(createInfo);
}

std::vector<vk::DescriptorSet> Device::allocateDescriptorSets(vk::DescriptorPool pool, vk::ArrayProxy<vk::DescriptorSetLayout> layouts,
															  u32 setCount)
{
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = setCount;
	allocInfo.pSetLayouts = layouts.data();
	return device.allocateDescriptorSets(allocInfo);
}

vk::RenderPass Device::createRenderPass(vk::ArrayProxy<vk::AttachmentDescription const> attachments,
										vk::ArrayProxy<vk::SubpassDescription const> subpasses,
										vk::ArrayProxy<vk::SubpassDependency> dependencies)
{
	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = attachments.size();
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = subpasses.size();
	createInfo.pSubpasses = subpasses.data();
	createInfo.dependencyCount = dependencies.size();
	createInfo.pDependencies = dependencies.data();
	return device.createRenderPass(createInfo);
}

vk::Framebuffer Device::createFramebuffer(vk::RenderPass renderPass, vk::ArrayProxy<vk::ImageView const> attachments, vk::Extent2D extent,
										  u32 layers)
{
	vk::FramebufferCreateInfo createInfo;
	createInfo.attachmentCount = attachments.size();
	createInfo.pAttachments = attachments.data();
	createInfo.renderPass = renderPass;
	createInfo.width = extent.width;
	createInfo.height = extent.height;
	createInfo.layers = layers;
	return device.createFramebuffer(createInfo);
}

Service::Service(InitInfo const& info)
{
	init(info);
}

Service::~Service()
{
	deinit();
}
} // namespace le::gfx
