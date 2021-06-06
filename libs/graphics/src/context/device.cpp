#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <core/maths.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
namespace {
template <typename T, typename U>
T const* fromNextChain(U* pNext, vk::StructureType type) {
	if (pNext) {
		auto* pIn = reinterpret_cast<vk::BaseInStructure const*>(pNext);
		if (pIn->sType == type) { return reinterpret_cast<T const*>(pIn); }
		return fromNextChain<T>(pIn->pNext, type);
	}
	return nullptr;
}
} // namespace

// Prevent validation spam on Windows
extern dl::level g_validationLevel;

Device::Device(not_null<Instance*> instance, vk::SurfaceKHR surface, CreateInfo const& info) : m_instance(instance) {
	if (default_v(instance->m_instance)) { throw std::runtime_error("Invalid graphics Instance"); }
	if (default_v(surface)) { throw std::runtime_error("Invalid Vulkan surface"); }
	// Prevent validation spam on Windows
	auto const validationLevel = std::exchange(g_validationLevel, dl::level::warning);
	std::unordered_set<std::string_view> const extSet = {info.extensions.begin(), info.extensions.end()};
	std::vector<std::string_view> const extArr = {extSet.begin(), extSet.end()};
	kt::fixed_vector<PhysicalDevice, 8> const devices = instance->availableDevices(extArr);
	if (devices.empty()) {
		g_log.log(lvl::error, 0, "[{}] No compatible Vulkan physical device detected!", g_name);
		throw std::runtime_error("No physical devices");
	}
	static DevicePicker const s_picker;
	DevicePicker const* pPicker = info.pPicker ? info.pPicker : &s_picker;
	PhysicalDevice picked = pPicker->pick(devices, info.pickOverride);
	if (default_v(picked.device)) { throw std::runtime_error("Failed to select a physical device!"); }
	m_physicalDevice = std::move(picked);
	m_metadata.available = std::move(devices);
	m_metadata.surface = surface;
	for (auto const& ext : extArr) { m_metadata.extensions.push_back(ext.data()); }
	m_metadata.limits = m_physicalDevice.properties.limits;
	m_metadata.lineWidth.first = m_physicalDevice.properties.limits.lineWidthRange[0U];
	m_metadata.lineWidth.second = m_physicalDevice.properties.limits.lineWidthRange[1U];
	auto families = utils::queueFamilies(m_physicalDevice, m_metadata.surface);
	if (info.qselect == QSelect::eSingleFamily || info.qselect == QSelect::eSingleQueue) {
		std::optional<QueueMultiplex::Family> uber;
		for (auto const& family : families) {
			if (family.flags.all(QFlags::inverse())) {
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
	if (!instance->m_metadata.layers.empty()) {
		deviceCreateInfo.enabledLayerCount = (u32)instance->m_metadata.layers.size();
		deviceCreateInfo.ppEnabledLayerNames = instance->m_metadata.layers.data();
	}
	deviceCreateInfo.enabledExtensionCount = (u32)m_metadata.extensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = m_metadata.extensions.data();
	m_device = m_physicalDevice.device.createDevice(deviceCreateInfo);
	m_queues.setup(m_device);
	instance->m_loader.init(m_device);
	g_log.log(lvl::info, 0, "[{}] Vulkan device constructed, using GPU {}", g_name, m_physicalDevice.toString());
	g_validationLevel = validationLevel;
}

Device::~Device() {
	waitIdle();
	if (!default_v(m_device)) { g_log.log(lvl::info, 1, "[{}] Vulkan device destroyed", g_name); }
	destroy(m_metadata.surface, m_device);
}

bool Device::valid(vk::SurfaceKHR surface) const { return m_physicalDevice.surfaceSupport(m_queues.familyIndex(QType::ePresent), surface); }

void Device::waitIdle() {
	if (!default_v(m_device)) { m_device.waitIdle(); }
	m_deferred.flush();
}

vk::Semaphore Device::makeSemaphore() const { return m_device.createSemaphore({}); }

vk::Fence Device::makeFence(bool bSignalled) const {
	vk::FenceCreateFlags flags = bSignalled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlags();
	return m_device.createFence(flags);
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
			auto const result = m_device.waitForFences(optional, true, s_wait);
			ENSURE(result != vk::Result::eTimeout && result != vk::Result::eErrorDeviceLost, "Fence wait failure!");
			if (result == vk::Result::eTimeout || result == vk::Result::eErrorDeviceLost) { g_log.log(lvl::error, 1, "[{}] Fence wait failure!", g_name); }
		} else {
			m_device.waitForFences(optional, true, maths::max<u64>());
		}
	}
}

void Device::waitAll(vAP<vk::Fence> validFences) const {
	if (!validFences.empty()) {
		if constexpr (levk_debug) {
			static constexpr u64 s_wait = 1000ULL * 1000 * 5000;
			auto const result = m_device.waitForFences(std::move(validFences), true, s_wait);
			ENSURE(result != vk::Result::eTimeout && result != vk::Result::eErrorDeviceLost, "Fence wait failure!");
			if (result == vk::Result::eTimeout || result == vk::Result::eErrorDeviceLost) { g_log.log(lvl::error, 1, "[{}] Fence wait failure!", g_name); }
		} else {
			m_device.waitForFences(std::move(validFences), true, maths::max<u64>());
		}
	}
}

void Device::resetFence(vk::Fence optional) const {
	if (!default_v(optional)) { m_device.resetFences(optional); }
}

void Device::resetAll(vAP<vk::Fence> validFences) const {
	if (!validFences.empty()) { m_device.resetFences(std::move(validFences)); }
}

bool Device::signalled(Span<vk::Fence const> fences) const {
	auto const s = [this](vk::Fence const& fence) -> bool { return default_v(fence) || m_device.getFenceStatus(fence) == vk::Result::eSuccess; };
	return std::all_of(fences.begin(), fences.end(), s);
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
	return m_device.createImageView(createInfo);
}

vk::PipelineCache Device::makePipelineCache() const { return m_device.createPipelineCache({}); }

vk::PipelineLayout Device::makePipelineLayout(vAP<vk::PushConstantRange> pushConstants, vAP<vk::DescriptorSetLayout> setLayouts) const {
	vk::PipelineLayoutCreateInfo createInfo;
	createInfo.setLayoutCount = setLayouts.size();
	createInfo.pSetLayouts = setLayouts.data();
	createInfo.pushConstantRangeCount = pushConstants.size();
	createInfo.pPushConstantRanges = pushConstants.data();
	return m_device.createPipelineLayout(createInfo);
}

vk::DescriptorSetLayout Device::makeDescriptorSetLayout(vAP<vk::DescriptorSetLayoutBinding> bindings) const {
	vk::DescriptorSetLayoutCreateInfo createInfo;
	createInfo.bindingCount = bindings.size();
	createInfo.pBindings = bindings.data();
	return m_device.createDescriptorSetLayout(createInfo);
}

vk::DescriptorPool Device::makeDescriptorPool(vAP<vk::DescriptorPoolSize> poolSizes, u32 maxSets) const {
	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.poolSizeCount = poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = maxSets;
	return m_device.createDescriptorPool(createInfo);
}

std::vector<vk::DescriptorSet> Device::allocateDescriptorSets(vk::DescriptorPool pool, vAP<vk::DescriptorSetLayout> layouts, u32 setCount) const {
	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = setCount;
	allocInfo.pSetLayouts = layouts.data();
	return m_device.allocateDescriptorSets(allocInfo);
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
	return m_device.createRenderPass(createInfo);
}

vk::Framebuffer Device::makeFramebuffer(vk::RenderPass renderPass, vAP<vk::ImageView> attachments, vk::Extent2D extent, u32 layers) const {
	vk::FramebufferCreateInfo createInfo;
	createInfo.attachmentCount = attachments.size();
	createInfo.pAttachments = attachments.data();
	createInfo.renderPass = renderPass;
	createInfo.width = extent.width;
	createInfo.height = extent.height;
	createInfo.layers = layers;
	return m_device.createFramebuffer(createInfo);
}

bool Device::setDebugUtilsName([[maybe_unused]] vk::DebugUtilsObjectNameInfoEXT const& info) const {
#if !defined(__ANDROID__)
	if (!default_v(m_instance->m_messenger)) {
		m_device.setDebugUtilsObjectNameEXT(info, m_instance->loader());
		return true;
	}
#endif
	return false;
}

bool Device::setDebugUtilsName(u64 handle, vk::ObjectType type, std::string_view name) const {
	vk::DebugUtilsObjectNameInfoEXT info;
	info.objectHandle = handle;
	info.objectType = type;
	info.pObjectName = name.data();
	return setDebugUtilsName(info);
}

void Device::defer(Deferred::Callback callback, u64 defer) { m_deferred.defer({std::move(callback), defer}); }

void Device::decrementDeferred() { m_deferred.decrement(); }
} // namespace le::graphics
