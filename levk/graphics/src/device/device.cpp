#include <device/device_impl.hpp>
#include <levk/core/build_version.hpp>
#include <levk/core/log_channel.hpp>
#include <levk/core/maths.hpp>
#include <levk/core/utils/data_store.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/core/utils/sys_info.hpp>
#include <levk/graphics/common.hpp>
#include <levk/graphics/device/device.hpp>
#include <levk/graphics/utils/utils.hpp>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace le::graphics {
namespace {
LogLevel g_validationLevel = LogLevel::warn;

struct vkLoader {
	vk::DynamicLoader dl;
	vk::DispatchLoaderDynamic dld;

	static std::optional<vkLoader> make() {
		vkLoader ret;
		auto addr = ret.dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
		if (!addr) { return std::nullopt; }
		ret.dld.init(addr);
		return ret;
	}
};

struct vkInst {
	vk::UniqueInstance instance;
	vk::UniqueDebugUtilsMessengerEXT messenger;
	std::vector<char const*> layers;
	std::vector<char const*> extensions;
};

void validationLog(LogLevel level, LogChannelMask channel, std::string_view msg) {
	static constexpr std::string_view name = "vk::validation";
	if (level == LogLevel::error || g_validationLevel <= level) { dlog::log(level, channel, "[{}] {}", name, msg); }
}

bool skipError(std::string_view msg) noexcept {
	static constexpr std::string_view skip[] = {"VkSwapchainCreateInfoKHR-imageExtent"};
	if (!msg.empty()) {
		for (auto str : skip) {
			if (msg.find(str) < msg.size()) { return true; }
		}
	}
	return false;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL validationCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
													VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void*) {
	std::string_view const msg = pCallbackData && pCallbackData->pMessage ? pCallbackData->pMessage : "UNKNOWN";
	using lvl = LogLevel;
	switch (messageSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
		validationLog(lvl::error, LC_EndUser, msg);
		EXPECT(false);
		return !skipError(msg);
	}
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: validationLog(lvl::warn, LC_LibUser, msg); break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: validationLog(lvl::debug, LC_Library, msg); break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: validationLog(lvl::info, LC_LibUser, msg); break;
	default: break;
	}
	return false;
}

bool findLayer(std::vector<vk::LayerProperties> const& available, char const* szLayer, std::optional<LogLevel> log) {
	std::string_view const layerName(szLayer);
	for (auto& layer : available) {
		if (std::string_view(layer.layerName) == layerName) { return true; }
	}
	if (log) { dlog::log(*log, "[{}] Requested layer [{}] not available!", g_name, szLayer); }
	return false;
}

vkInst makeInstance(Device::CreateInfo const& info) {
	static constexpr char const* szValidationLayer = "VK_LAYER_KHRONOS_validation";
	vk::DynamicLoader dl;
	VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	auto const layerProps = vk::enumerateInstanceLayerProperties();
	std::unordered_set<std::string_view> requiredExtensionsSet = {info.instance.extensions.begin(), info.instance.extensions.end()};
	Validation validation = info.instance.validation;
	if (auto vd = DataObject<Validation>("validation")) {
		validation = *vd;
		logI(LC_LibUser, "[{}] Forcing validation layers: {}", g_name, validation == Validation::eOn ? "on" : "off");
	}
	vkInst ret;
	if (validation == Validation::eOn) {
		bool const validationLayerFound = findLayer(layerProps, szValidationLayer, LogLevel::warn);
		EXPECT(validationLayerFound);
		if (validationLayerFound) {
			requiredExtensionsSet.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			ret.layers.push_back(szValidationLayer);
		} else {
			validation = Validation::eOff;
		}
	}
	for (auto ext : requiredExtensionsSet) { ret.extensions.push_back(ext.data()); }

#if defined(LEVK_OS_APPLE)
	ret.extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = info.app.name.data();
	appInfo.applicationVersion = VK_MAKE_VERSION(info.app.version.major(), info.app.version.minor(), info.app.version.patch());
	appInfo.pEngineName = "LittleEngineVk";
	auto const& ver = g_buildVersion.version;
	appInfo.engineVersion = VK_MAKE_VERSION(ver.major(), ver.minor(), ver.patch());
	appInfo.apiVersion = VK_API_VERSION_1_0;
	vk::InstanceCreateInfo createInfo;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.ppEnabledExtensionNames = ret.extensions.data();
	createInfo.enabledExtensionCount = (u32)ret.extensions.size();
	createInfo.ppEnabledLayerNames = ret.layers.data();
	createInfo.enabledLayerCount = (u32)ret.layers.size();
	ret.instance = vk::createInstanceUnique(createInfo, nullptr);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*ret.instance);
	if (validation == Validation::eOn) {
		vk::DebugUtilsMessengerCreateInfoEXT createInfo;
		using vksev = vk::DebugUtilsMessageSeverityFlagBitsEXT;
		createInfo.messageSeverity = vksev::eError | vksev::eWarning | vksev::eInfo | vksev::eVerbose;
		using vktype = vk::DebugUtilsMessageTypeFlagBitsEXT;
		createInfo.messageType = vktype::eGeneral | vktype::ePerformance | vktype::eValidation;
		createInfo.pfnUserCallback = &validationCallback;
		ENSURE(VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateDebugUtilsMessengerEXT, "Function pointer is null");
		ret.messenger = ret.instance->createDebugUtilsMessengerEXTUnique(createInfo, nullptr);
	}
	logI(LC_LibUser, "[{}] Vulkan instance constructed", g_name);
	g_validationLevel = info.validationLogLevel;
	return ret;
}

template <typename T>
ktl::fixed_vector<PhysicalDevice, 8> validDevices(T const& extensions, vk::Instance instance, vk::DispatchLoaderDynamic const& d) {
	ktl::fixed_vector<PhysicalDevice, 8> ret;
	std::vector<vk::PhysicalDevice> const devices = instance.enumeratePhysicalDevices(d);
	std::unordered_set<std::string_view> extSet;
	for (auto const& ext : extensions) { extSet.insert(ext); }
	for (auto const& device : devices) {
		std::unordered_set<std::string_view> missing = extSet;
		std::vector<vk::ExtensionProperties> const supported = device.enumerateDeviceExtensionProperties(nullptr, d);
		for (std::size_t idx = 0; idx < supported.size() && !missing.empty(); ++idx) { missing.erase(std::string_view(supported[idx].extensionName)); }
		if (missing.empty()) {
			PhysicalDevice available;
			available.properties = device.getProperties(d);
			available.memoryProperties = device.getMemoryProperties();
			available.queueFamilies = device.getQueueFamilyProperties(d);
			available.features = device.getFeatures(d);
			available.device = device;
			ret.push_back(std::move(available));
			if (ret.size() == ret.capacity()) { break; }
		}
	}
	return ret;
}

std::size_t deviceIndex(Span<PhysicalDevice const> devices, std::string_view name) {
	EXPECT(!devices.empty());
	if (!name.empty()) {
		for (std::size_t i = 0; i < devices.size(); ++i) {
			if (devices[i].name() == name) { return i; }
		}
	}
	for (std::size_t i = 0; i < devices.size(); ++i) {
		if (devices[i].discreteGPU()) { return i; }
	}
	return 0U;
}
} // namespace

ktl::fixed_vector<PhysicalDevice, 8> Device::physicalDevices() {
	if (auto loader = vkLoader::make()) {
		auto instance = vk::createInstanceUnique({}, nullptr, loader->dld);
		loader->dld.init(*instance);
		return validDevices(requiredExtensions, *instance, loader->dld);
	}
	return {};
}

Device::Device(Impl&& impl) noexcept : m_impl(std::move(impl)) {}

std::unique_ptr<Device> Device::make(CreateInfo const& info, Device::MakeSurface&& makeSurface) {
	if (!makeSurface) {
		logE(LC_LibUser, "[{}] Invalid MakeSurface instance", g_name);
		return {};
	}
	Impl impl;
	impl.ftLib = FTUnique<FTLib>(FTLib::make());
	if (!impl.ftLib) {
		logE(LC_LibUser, "[{}] Failed to initialize Freetype", g_name);
		return {};
	}
	auto instance = makeInstance(info);
	vk::UniqueSurfaceKHR surface;
	{
		auto sf = makeSurface(*instance.instance);
		if (!sf) {
			logE(LC_LibUser, "[{}] Failed to make Vulkan Surface", g_name);
			return {};
		}
		surface = vk::UniqueSurfaceKHR(sf, *instance.instance);
	}
	// Prevent validation spam on Windows
	auto const validationLevel = std::exchange(g_validationLevel, LogLevel::warn);
	std::vector<char const*> extensions;
	extensions.reserve(info.extensions.size() + std::size(requiredExtensions));
	for (auto const ext : info.extensions) { extensions.push_back(ext.data()); }
	for (auto const ext : requiredExtensions) { extensions.push_back(ext.data()); }
	auto const devices = validDevices(extensions, *instance.instance, VULKAN_HPP_DEFAULT_DISPATCHER);
	if (devices.empty()) {
		logE(LC_EndUser, "[{}] No compatible Vulkan GPU detected!", g_name);
		return {};
	}
	auto const index = deviceIndex(devices, info.customDeviceName);
	EXPECT(index < devices.size());
	if (!devices[index].device) {
		logE(LC_LibUser, "[{}] Failed to select a physical device!", g_name);
		return {};
	}
	PhysicalDevice const& picked = devices[index];
	auto const queueSelect = Queues::select(picked, *surface);
	vk::PhysicalDeviceFeatures deviceFeatures;
	deviceFeatures.fillModeNonSolid = picked.features.fillModeNonSolid;
	deviceFeatures.wideLines = picked.features.wideLines;
	deviceFeatures.samplerAnisotropy = picked.features.samplerAnisotropy;
	vk::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = (u32)queueSelect.dqci.size();
	deviceCreateInfo.pQueueCreateInfos = queueSelect.dqci.data();
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledLayerCount = (u32)instance.layers.size();
	deviceCreateInfo.ppEnabledLayerNames = instance.layers.data();
	deviceCreateInfo.enabledExtensionCount = (u32)extensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = extensions.data();
	auto device = picked.device.createDeviceUnique(deviceCreateInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
	if (!picked.surfaceSupport(queueSelect.info[queueSelect.primary].family, *surface)) {
		logE(LC_LibUser, "[{}] Vulkan surface does not support presentation", g_name);
		return {};
	}
	auto ret = std::unique_ptr<Device>(new Device(std::move(impl)));
	if (!ret->m_queues.setup(*device, queueSelect)) {
		logE(LC_LibUser, "[{}] Failed to setup Vulkan queues!", g_name);
		return {};
	}

	g_validationLevel = validationLevel;
	DataStore::getOrSet<::le::utils::SysInfo>("sys_info").gpuName = std::string(picked.name());
	ret->m_makeSurface = std::move(makeSurface);
	ret->m_instance = std::move(instance.instance);
	ret->m_messenger = std::move(instance.messenger);
	ret->m_device = std::move(device);
	ret->m_physicalDeviceIndex = index;
	ret->m_metadata.available = std::move(devices);
	ret->m_metadata.extensions = std::move(extensions);
	ret->m_metadata.limits = picked.properties.limits;

	logI(LC_LibUser, "[{}] Vulkan device constructed, using GPU {}", g_name, picked.toString());
	return ret;
}

Device::~Device() {
	waitIdle();
	logI(LC_LibUser, "[{}] Vulkan device destroyed", g_name);
}

bool Device::valid(vk::SurfaceKHR surface) const { return physicalDevice().surfaceSupport(m_queues.graphics().family(), surface); }

void Device::waitIdle() {
	m_device->waitIdle();
	m_deferred.flush();
}

vk::UniqueSurfaceKHR Device::makeSurface() const {
	auto ret = m_makeSurface(*m_instance);
	ENSURE(valid(ret), "Invalid surface");
	return vk::UniqueSurfaceKHR(ret, *m_instance);
}

vk::Semaphore Device::makeSemaphore() const { return m_device->createSemaphore({}); }

vk::Fence Device::makeFence(bool signalled) const {
	vk::FenceCreateFlags flags = signalled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlags();
	return m_device->createFence(flags);
}

void Device::resetOrMakeFence(vk::Fence& out_fence, bool signalled) const {
	if (default_v(out_fence)) {
		out_fence = makeFence(signalled);
	} else {
		resetFence(out_fence, true);
	}
}

bool Device::isBusy(vk::Fence fence) const {
	if (fence) { return m_device->getFenceStatus(fence) == vk::Result::eNotReady; }
	return false;
}

void Device::waitFor(Span<vk::Fence const> fences, stdch::nanoseconds const wait) const {
	if constexpr (levk_debug) {
		auto const result = m_device->waitForFences(u32(fences.size()), fences.data(), true, static_cast<u64>(wait.count()));
		ENSURE(result != vk::Result::eTimeout && result != vk::Result::eErrorDeviceLost, "Fence wait failure!");
	} else {
		m_device->waitForFences(u32(fences.size()), fences.data(), true, maths::max<u64>());
	}
}

void Device::resetFence(vk::Fence optional, bool wait) const {
	if (optional) {
		if (wait && m_device->getFenceStatus(optional) == vk::Result::eNotReady) { waitFor(optional); }
		m_device->resetFences(optional);
	}
}

void Device::resetAll(Span<vk::Fence const> fences) const {
	if (!fences.empty()) { m_device->resetFences(u32(fences.size()), fences.data()); }
}

void Device::resetCommandPool(vk::CommandPool pool) const {
	if (!default_v(pool)) { m_device->resetCommandPool(pool, {}); }
}

bool Device::signalled(Span<vk::Fence const> fences) const {
	auto const s = [this](vk::Fence const& fence) -> bool { return default_v(fence) || m_device->getFenceStatus(fence) == vk::Result::eSuccess; };
	return std::all_of(fences.begin(), fences.end(), s);
}

vk::ImageView Device::makeImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, vk::ImageViewType type, u32 mipLevels) const {
	vk::ImageViewCreateInfo createInfo;
	createInfo.image = image;
	createInfo.viewType = type;
	createInfo.format = format;
	createInfo.components.r = createInfo.components.g = createInfo.components.b = createInfo.components.a = vk::ComponentSwizzle::eIdentity;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = mipLevels;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = type == vk::ImageViewType::eCube ? 6 : 1;
	return m_device->createImageView(createInfo);
}

vk::PipelineCache Device::makePipelineCache() const { return m_device->createPipelineCache({}); }

vk::PipelineLayout Device::makePipelineLayout(vAP<vk::PushConstantRange> pushConstants, vAP<vk::DescriptorSetLayout> setLayouts) const {
	vk::PipelineLayoutCreateInfo createInfo;
	createInfo.setLayoutCount = setLayouts.size();
	createInfo.pSetLayouts = setLayouts.data();
	createInfo.pushConstantRangeCount = pushConstants.size();
	createInfo.pPushConstantRanges = pushConstants.data();
	return m_device->createPipelineLayout(createInfo);
}

vk::DescriptorSetLayout Device::makeDescriptorSetLayout(vAP<vk::DescriptorSetLayoutBinding> bindings) const {
	vk::DescriptorSetLayoutCreateInfo createInfo;
	createInfo.bindingCount = bindings.size();
	createInfo.pBindings = bindings.data();
	return m_device->createDescriptorSetLayout(createInfo);
}

vk::DescriptorPool Device::makeDescriptorPool(Span<vk::DescriptorPoolSize const> poolSizes, u32 maxSets) const {
	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.poolSizeCount = poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = maxSets;
	return m_device->createDescriptorPool(createInfo);
}

std::vector<vk::DescriptorSet> Device::allocateDescriptorSets(vk::DescriptorPool pool, vAP<vk::DescriptorSetLayout> layouts, u32 setCount) const {
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = setCount;
	allocInfo.pSetLayouts = layouts.data();
	return m_device->allocateDescriptorSets(allocInfo);
}

vk::Framebuffer Device::makeFramebuffer(vk::RenderPass renderPass, Span<vk::ImageView const> attachments, vk::Extent2D extent, u32 layers) const {
	vk::FramebufferCreateInfo createInfo;
	createInfo.attachmentCount = (u32)attachments.size();
	createInfo.pAttachments = attachments.data();
	createInfo.renderPass = renderPass;
	createInfo.width = extent.width;
	createInfo.height = extent.height;
	createInfo.layers = layers;
	return m_device->createFramebuffer(createInfo);
}

vk::Sampler Device::makeSampler(vk::SamplerCreateInfo info) const { return m_device->createSampler(info); }

bool Device::setDebugUtilsName([[maybe_unused]] vk::DebugUtilsObjectNameInfoEXT const& info) const {
	if (!default_v(*m_messenger)) {
		m_device->setDebugUtilsObjectNameEXT(info);
		return true;
	}
	return false;
}

bool Device::setDebugUtilsName(u64 handle, vk::ObjectType type, std::string_view name) const {
	vk::DebugUtilsObjectNameInfoEXT info;
	info.objectHandle = handle;
	info.objectType = type;
	info.pObjectName = name.data();
	return setDebugUtilsName(info);
}

void Device::defer(DeferQueue::Callback&& callback, Buffering defer) { m_deferred.defer(std::move(callback), defer); }

void Device::decrementDeferred() { m_deferred.decrement(); }
} // namespace le::graphics
