#include <core/maths.hpp>
#include <core/utils/data_store.hpp>
#include <core/utils/sys_info.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/utils/utils.hpp>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_set>

namespace le::graphics {
namespace {
dl::level g_validationLevel = dl::level::warn;

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

void validationLog(dl::level level, int verbosity, std::string_view msg) {
	static constexpr std::string_view name = "vk::validation";
	if (level == dl::level::error || g_validationLevel <= level) { g_log.log(level, verbosity, "[{}] {}", name, msg); }
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
	switch (messageSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
		validationLog(lvl::error, 0, msg);
		bool const ret = !skipError(msg);
		ENSURE(!ret, "Validation error");
		return ret;
	}
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: validationLog(lvl::warn, 1, msg); break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: validationLog(lvl::debug, 2, msg); break;
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: validationLog(lvl::info, 1, msg); break;
	}
	return false;
}

bool findLayer(std::vector<vk::LayerProperties> const& available, char const* szLayer, std::optional<dl::level> log) {
	std::string_view const layerName(szLayer);
	for (auto& layer : available) {
		if (std::string_view(layer.layerName) == layerName) { return true; }
	}
	if (log) { dl::log(*log, "[{}] Requested layer [{}] not available!", g_name, szLayer); }
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
		g_log.log(lvl::info, 1, "[{}] Forcing validation layers: {}", g_name, validation == Validation::eOn ? "on" : "off");
	}
	vkInst ret;
	if (validation == Validation::eOn) {
		if (findLayer(layerProps, szValidationLayer, dl::level::warn)) {
			requiredExtensionsSet.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			ret.layers.push_back(szValidationLayer);
		} else {
			validation = Validation::eOff;
		}
	}
	for (auto ext : requiredExtensionsSet) { ret.extensions.push_back(ext.data()); }
	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = "LittleEngineVk Game";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "LittleEngineVk";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
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
	g_log.log(lvl::info, 1, "[{}] Vulkan instance constructed", g_name);
	g_validationLevel = info.logLevel;
	return ret;
}

ktl::fixed_vector<PhysicalDevice, 8> validDevices(Span<std::string_view const> extensions, vk::Instance instance, vk::DispatchLoaderDynamic const& d) {
	ktl::fixed_vector<PhysicalDevice, 8> ret;
	std::vector<vk::PhysicalDevice> const devices = instance.enumeratePhysicalDevices(d);
	std::unordered_set<std::string_view> const extSet = {extensions.begin(), extensions.end()};
	for (auto const& device : devices) {
		std::unordered_set<std::string_view> missing = extSet;
		std::vector<vk::ExtensionProperties> const supported = device.enumerateDeviceExtensionProperties(nullptr, d);
		for (std::size_t idx = 0; idx < supported.size() && !missing.empty(); ++idx) { missing.erase(std::string_view(supported[idx].extensionName)); }
		if (missing.empty()) {
			PhysicalDevice available;
			available.properties = device.getProperties(d);
			available.queueFamilies = device.getQueueFamilyProperties(d);
			available.features = device.getFeatures(d);
			available.device = device;
			ret.push_back(std::move(available));
			if (ret.size() == ret.capacity()) { break; }
		}
	}
	return ret;
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

Device::Device(CreateInfo const& info, Device::MakeSurface&& makeSurface) {
	if (!makeSurface) { throw std::runtime_error("Invalid makeSurface instance"); }
	auto instance = makeInstance(info);
	auto surface = makeSurface(*instance.instance);
	if (default_v(surface)) { throw std::runtime_error("Invalid Vulkan surface"); }
	m_uinstance = std::move(instance.instance);
	m_messenger = std::move(instance.messenger);
	// Prevent validation spam on Windows
	auto const validationLevel = std::exchange(g_validationLevel, dl::level::warn);
	std::vector<std::string_view> extensions = {info.extensions.begin(), info.extensions.end()};
	std::copy(requiredExtensions.begin(), requiredExtensions.end(), std::back_inserter(extensions));
	ktl::fixed_vector const devices = validDevices(extensions, *m_uinstance, VULKAN_HPP_DEFAULT_DISPATCHER);
	if (devices.empty()) {
		g_log.log(lvl::error, 0, "[{}] No compatible Vulkan physical device detected!", g_name);
		throw std::runtime_error("No physical devices");
	}
	static DevicePicker const s_picker;
	DevicePicker const* pPicker = info.picker ? info.picker : &s_picker;
	PhysicalDevice picked = pPicker->pick(devices, info.pickOverride);
	if (default_v(picked.device)) { throw std::runtime_error("Failed to select a physical device!"); }
	m_physicalDevice = std::move(picked);
	m_metadata.available = std::move(devices);
	DataStore::getOrSet<::le::utils::SysInfo>("sys_info").gpuName = std::string(m_physicalDevice.name());
	m_surface = surface;
	for (auto const& ext : extensions) { m_metadata.extensions.push_back(ext.data()); }
	m_metadata.limits = m_physicalDevice.properties.limits;
	m_metadata.lineWidth.first = m_physicalDevice.properties.limits.lineWidthRange[0U];
	m_metadata.lineWidth.second = m_physicalDevice.properties.limits.lineWidthRange[1U];
	auto families = utils::queueFamilies(m_physicalDevice, m_surface);
	if (info.qselect == QSelect::eSingleFamily || info.qselect == QSelect::eSingleQueue) {
		std::optional<QueueMultiplex::Family> uber;
		for (auto const& family : families) {
			if (family.flags.all(qflags_all)) {
				uber = family;
				g_log.log(lvl::info, 1, "[{}] Forcing single Vulkan queue family [{}]", g_name, family.familyIndex);
				break;
			}
		}
		if (uber) {
			if (info.qselect == QSelect::eSingleQueue) {
				g_log.log(lvl::info, 1, "[{}] Forcing single Vulkan queue (family supports [{}])", g_name, uber->total);
				uber->total = 1;
			}
			families = {*uber};
		}
	}
	auto queueCreateInfos = m_queues.select(families);
	vk::PhysicalDeviceFeatures deviceFeatures;
	deviceFeatures.fillModeNonSolid = m_physicalDevice.features.fillModeNonSolid;
	deviceFeatures.wideLines = m_physicalDevice.features.wideLines;
	vk::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = (u32)queueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledLayerCount = (u32)instance.layers.size();
	deviceCreateInfo.ppEnabledLayerNames = instance.layers.data();
	deviceCreateInfo.enabledExtensionCount = (u32)m_metadata.extensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = m_metadata.extensions.data();
	m_device = m_physicalDevice.device.createDeviceUnique(deviceCreateInfo);
	m_queues.setup(*m_device);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device);
	g_log.log(lvl::info, 0, "[{}] Vulkan device constructed, using GPU {}", g_name, m_physicalDevice.toString());
	g_validationLevel = validationLevel;
}

Device::~Device() {
	waitIdle();
	if (!default_v(m_surface)) { m_uinstance->destroy(m_surface); }
	g_log.log(lvl::info, 1, "[{}] Vulkan device destroyed", g_name);
}

bool Device::valid(vk::SurfaceKHR surface) const { return m_physicalDevice.surfaceSupport(m_queues.familyIndex(QType::ePresent), surface); }

void Device::waitIdle() {
	m_device->waitIdle();
	m_deferred.flush();
}

vk::Semaphore Device::makeSemaphore() const { return m_device->createSemaphore({}); }

vk::Fence Device::makeFence(bool bSignalled) const {
	vk::FenceCreateFlags flags = bSignalled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlags();
	return m_device->createFence(flags);
}

void Device::resetOrMakeFence(vk::Fence& out_fence, bool bSignalled) const {
	if (default_v(out_fence)) {
		out_fence = makeFence(bSignalled);
	} else {
		resetFence(out_fence);
	}
}

void Device::waitFor(vk::Fence optional) const {
	if (!default_v(optional)) {
		if constexpr (levk_debug) {
			static constexpr u64 s_wait = 1000ULL * 1000 * 5000;
			auto const result = m_device->waitForFences(optional, true, s_wait);
			ENSURE(result != vk::Result::eTimeout && result != vk::Result::eErrorDeviceLost, "Fence wait failure!");
			if (result == vk::Result::eTimeout || result == vk::Result::eErrorDeviceLost) { g_log.log(lvl::error, 1, "[{}] Fence wait failure!", g_name); }
		} else {
			m_device->waitForFences(optional, true, maths::max<u64>());
		}
	}
}

void Device::waitAll(vAP<vk::Fence> validFences) const {
	if (!validFences.empty()) {
		if constexpr (levk_debug) {
			static constexpr u64 s_wait = 1000ULL * 1000 * 5000;
			auto const result = m_device->waitForFences(std::move(validFences), true, s_wait);
			ENSURE(result != vk::Result::eTimeout && result != vk::Result::eErrorDeviceLost, "Fence wait failure!");
			if (result == vk::Result::eTimeout || result == vk::Result::eErrorDeviceLost) { g_log.log(lvl::error, 1, "[{}] Fence wait failure!", g_name); }
		} else {
			m_device->waitForFences(std::move(validFences), true, maths::max<u64>());
		}
	}
}

void Device::resetFence(vk::Fence optional) const {
	if (!default_v(optional)) { m_device->resetFences(optional); }
}

void Device::resetAll(vAP<vk::Fence> validFences) const {
	if (!validFences.empty()) { m_device->resetFences(std::move(validFences)); }
}

bool Device::signalled(Span<vk::Fence const> fences) const {
	auto const s = [this](vk::Fence const& fence) -> bool { return default_v(fence) || m_device->getFenceStatus(fence) == vk::Result::eSuccess; };
	return std::all_of(fences.begin(), fences.end(), s);
}

vk::CommandPool Device::makeCommandPool(vk::CommandPoolCreateFlags flags, QType qtype) const {
	vk::CommandPoolCreateInfo info(flags, m_queues.familyIndex(qtype));
	return m_device->createCommandPool(info);
}

vk::ImageView Device::makeImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, vk::ImageViewType type) const {
	vk::ImageViewCreateInfo createInfo;
	createInfo.image = image;
	createInfo.viewType = type;
	createInfo.format = format;
	createInfo.components.r = createInfo.components.g = createInfo.components.b = createInfo.components.a = vk::ComponentSwizzle::eIdentity;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
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

vk::DescriptorPool Device::makeDescriptorPool(vAP<vk::DescriptorPoolSize> poolSizes, u32 maxSets) const {
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

vk::RenderPass Device::makeRenderPass(vAP<vk::AttachmentDescription> attachments, vAP<vk::SubpassDescription> subpasses,
									  vAP<vk::SubpassDependency> dependencies) const {
	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = attachments.size();
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = subpasses.size();
	createInfo.pSubpasses = subpasses.data();
	createInfo.dependencyCount = dependencies.size();
	createInfo.pDependencies = dependencies.data();
	return m_device->createRenderPass(createInfo);
}

vk::Framebuffer Device::makeFramebuffer(vk::RenderPass renderPass, vAP<vk::ImageView> attachments, vk::Extent2D extent, u32 layers) const {
	vk::FramebufferCreateInfo createInfo;
	createInfo.attachmentCount = attachments.size();
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
