#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <core/maths.hpp>
#include <graphics/context/device.hpp>

namespace le::graphics {
namespace {
struct QueueFamily final {
	u32 index = 0;
	u32 queueCount = 0;
	QFlags flags;
};

void listDevices(Span<AvailableDevice> devices) {
	std::stringstream str;
	str << "\nAvailable GPUs:";
	std::size_t idx = 0;
	for (auto const& device : devices) {
		str << " [" << idx << "] " << device.name() << '\t';
	}
	str << "\n\n";
	std::cout << str.str();
}
} // namespace

// Prevent validation spam on Windows
extern dl::level g_validationLevel;

Device::Device(Instance& instance, vk::SurfaceKHR surface, CreateInfo const& info) : m_instance(instance) {
	if (default_v(instance.m_instance)) {
		throw std::runtime_error("Invalid graphics Instance");
	}
	if (default_v(surface)) {
		throw std::runtime_error("Invalid Vulkan surface");
	}
	// Prevent validation spam on Windows
	auto const validationLevel = std::exchange(g_validationLevel, dl::level::warning);
	m_metadata.surface = surface;
	m_metadata.available = availableDevices();
	if (info.bPrintAvailable) {
		listDevices(m_metadata.available);
	}
	if (info.pickDevice) {
		m_metadata.picked = info.pickDevice(m_metadata.available);
		if (!default_v(m_metadata.picked.physicalDevice)) {
			logI("[{}] Using custom GPU: {}", g_name, m_metadata.picked.name());
		}
	}
	if (default_v(m_metadata.picked.physicalDevice) && !m_metadata.available.empty()) {
		for (auto const& availableDevice : m_metadata.available) {
			if (availableDevice.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
				m_metadata.picked = availableDevice;
				break;
			}
		}
		if (default_v(m_metadata.picked.physicalDevice)) {
			m_metadata.picked = m_metadata.available.front();
		}
	}
	if (default_v(m_metadata.picked.physicalDevice)) {
		throw std::runtime_error("Failed to select a physical device!");
	}
	m_physicalDevice = m_metadata.picked.physicalDevice;
	m_metadata.limits = m_metadata.picked.properties.limits;
	m_metadata.lineWidth.first = m_metadata.picked.properties.limits.lineWidthRange[0];
	m_metadata.lineWidth.second = m_metadata.picked.properties.limits.lineWidthRange[1];
	// TODO
	// rd::ImageSamplers::clampDiffSpecCount(instance.deviceLimits.maxPerStageDescriptorSamplers);

	auto const queueFamilyProperties = m_metadata.picked.queueFamilies;
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::unordered_map<u32, QueueFamily> queueFamilies;
	QFlags found;
	std::size_t graphicsFamilyIdx = 0;
	for (std::size_t idx = 0; idx < queueFamilyProperties.size() && !found.bits.all(); ++idx) {
		auto const& queueFamily = queueFamilyProperties[idx];
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			if (!found.test(QType::eGraphics)) {
				QueueFamily& family = queueFamilies[(u32)idx];
				family.index = (u32)idx;
				family.queueCount = queueFamily.queueCount;
				family.flags |= QType::eGraphics;
				found.set(QType::eGraphics);
				graphicsFamilyIdx = idx;
			}
		}
		if (m_physicalDevice.getSurfaceSupportKHR((u32)idx, m_metadata.surface, instance.m_loader)) {
			if (!found.test(QType::ePresent)) {
				QueueFamily& family = queueFamilies[(u32)idx];
				family.index = (u32)idx;
				family.queueCount = queueFamily.queueCount;
				family.flags |= QType::ePresent;
				found.set(QType::ePresent);
			}
		}
		if (info.bDedicatedTransfer && queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) {
			if (found.test(QType::eGraphics) && idx != graphicsFamilyIdx && !found.test(QType::eTransfer)) {
				QueueFamily& family = queueFamilies[(u32)idx];
				family.index = (u32)idx;
				family.queueCount = queueFamily.queueCount;
				family.flags |= QType::eTransfer;
				found.set(QType::eTransfer);
			}
		}
	}
	if (!found.test(QType::eGraphics) || !found.test(QType::ePresent)) {
		throw std::runtime_error("Failed to obtain graphics/present queues from device!");
	}
	EnumArray<QType, Queues::Indices> queueData;
	f32 priority = 1.0f;
	f32 const priorities[] = {0.7f, 0.3f};
	for (auto& [index, queueFamily] : queueFamilies) {
		vk::DeviceQueueCreateInfo queueCreateInfo;
		queueCreateInfo.queueFamilyIndex = index;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &priority;
		if (queueFamily.flags.test(QType::eGraphics)) {
			queueData[(std::size_t)QType::eGraphics].familyIndex = index;
		}
		if (queueFamily.flags.test(QType::ePresent)) {
			queueData[(std::size_t)QType::ePresent].familyIndex = index;
		}
		if (queueFamily.flags.test(QType::eTransfer)) {
			queueData[(std::size_t)QType::eTransfer].familyIndex = index;
		}
		if (info.bDedicatedTransfer && !found.test(QType::eTransfer) && queueFamily.flags.test(QType::eGraphics) && queueFamily.queueCount > 1) {
			queueCreateInfo.queueCount = 2;
			queueCreateInfo.pQueuePriorities = priorities;
			queueData[(std::size_t)QType::eTransfer].familyIndex = 1;
			found.set(QType::eTransfer);
		}
		queueCreateInfos.push_back(std::move(queueCreateInfo));
	}
	if (!found.test(QType::eTransfer)) {
		queueData[(std::size_t)QType::eTransfer] = queueData[(std::size_t)QType::eGraphics];
	}
	vk::PhysicalDeviceFeatures deviceFeatures;
	deviceFeatures.fillModeNonSolid = true;
	deviceFeatures.wideLines = m_metadata.lineWidth.second > 1.0f;
	vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures;
	descriptorIndexingFeatures.runtimeDescriptorArray = true;
	vk::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = (u32)queueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.pNext = &descriptorIndexingFeatures;
	if (!instance.m_metadata.layers.empty()) {
		deviceCreateInfo.enabledLayerCount = (u32)instance.m_metadata.layers.size();
		deviceCreateInfo.ppEnabledLayerNames = instance.m_metadata.layers.data();
	}
	deviceCreateInfo.enabledExtensionCount = (u32)requiredExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();
	m_device = m_physicalDevice.createDevice(deviceCreateInfo);
	m_queues.setup(m_device, queueData);
	logD("[{}] Vulkan device constructed", g_name);
	g_validationLevel = validationLevel;
}

std::vector<AvailableDevice> Device::availableDevices() const {
	std::vector<AvailableDevice> ret;
	Instance& inst = m_instance;
	auto physicalDevices = inst.m_instance.enumeratePhysicalDevices();
	ret.reserve(physicalDevices.size());
	for (auto const& physDev : physicalDevices) {
		std::unordered_set<std::string_view> missingExtensions(requiredExtensions.begin(), requiredExtensions.end());
		auto const extensions = physDev.enumerateDeviceExtensionProperties();
		for (std::size_t idx = 0; idx < extensions.size() && !missingExtensions.empty(); ++idx) {
			missingExtensions.erase(std::string_view(extensions[idx].extensionName));
		}
		if (missingExtensions.empty()) {
			AvailableDevice availableDevice;
			availableDevice.properties = physDev.getProperties();
			availableDevice.queueFamilies = physDev.getQueueFamilyProperties();
			availableDevice.features = physDev.getFeatures();
			availableDevice.physicalDevice = physDev;
			ret.push_back(std::move(availableDevice));
		}
	}
	return ret;
}

Device::~Device() {
	waitIdle();
	logD_if(!default_v(m_device), "[{}] Vulkan device destroyed", g_name);
	destroy(m_metadata.surface, m_device);
}

bool Device::valid(vk::SurfaceKHR surface) const {
	if (!default_v(m_physicalDevice)) {
		return m_physicalDevice.getSurfaceSupportKHR(m_queues.familyIndex(QType::ePresent), surface);
	}
	return false;
}

void Device::waitIdle() {
	if (!default_v(m_device)) {
		m_device.waitIdle();
	}
	m_deferred.flush();
}

vk::Semaphore Device::createSemaphore() const {
	return m_device.createSemaphore({});
}

vk::Fence Device::createFence(bool bSignalled) const {
	vk::FenceCreateFlags flags = bSignalled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlags();
	return m_device.createFence(flags);
}

void Device::resetOrCreateFence(vk::Fence& out_fence, bool bSignalled) const {
	if (default_v(out_fence)) {
		out_fence = createFence(bSignalled);
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
			if (result == vk::Result::eTimeout || result == vk::Result::eErrorDeviceLost) {
				logE("[{}] Fence wait failure!", g_name);
			}
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
			if (result == vk::Result::eTimeout || result == vk::Result::eErrorDeviceLost) {
				logE("[{}] Fence wait failure!", g_name);
			}
		} else {
			m_device.waitForFences(std::move(validFences), true, maths::max<u64>());
		}
	}
}

void Device::resetFence(vk::Fence optional) const {
	if (!default_v(optional)) {
		m_device.resetFences(optional);
	}
}

void Device::resetAll(vAP<vk::Fence> validFences) const {
	if (!validFences.empty()) {
		m_device.resetFences(std::move(validFences));
	}
}

bool Device::signalled(Span<vk::Fence> fences) const {
	auto const s = [this](vk::Fence const& fence) -> bool { return default_v(fence) || m_device.getFenceStatus(fence) == vk::Result::eSuccess; };
	return std::all_of(fences.begin(), fences.end(), s);
}

vk::ImageView Device::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, vk::ImageViewType type) const {
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

vk::PipelineLayout Device::createPipelineLayout(vAP<vk::PushConstantRange> pushConstants, vAP<vk::DescriptorSetLayout> setLayouts) const {
	vk::PipelineLayoutCreateInfo createInfo;
	createInfo.setLayoutCount = setLayouts.size();
	createInfo.pSetLayouts = setLayouts.data();
	createInfo.pushConstantRangeCount = pushConstants.size();
	createInfo.pPushConstantRanges = pushConstants.data();
	return m_device.createPipelineLayout(createInfo);
}

vk::DescriptorSetLayout Device::createDescriptorSetLayout(vAP<vk::DescriptorSetLayoutBinding> bindings) const {
	vk::DescriptorSetLayoutCreateInfo createInfo;
	createInfo.bindingCount = bindings.size();
	createInfo.pBindings = bindings.data();
	return m_device.createDescriptorSetLayout(createInfo);
}

vk::DescriptorPool Device::createDescriptorPool(vAP<vk::DescriptorPoolSize> poolSizes, u32 maxSets) const {
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

vk::RenderPass Device::createRenderPass(vAP<vk::AttachmentDescription> attachments, vAP<vk::SubpassDescription> subpasses,
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

vk::Framebuffer Device::createFramebuffer(vk::RenderPass renderPass, vAP<vk::ImageView> attachments, vk::Extent2D extent, u32 layers) const {
	vk::FramebufferCreateInfo createInfo;
	createInfo.attachmentCount = attachments.size();
	createInfo.pAttachments = attachments.data();
	createInfo.renderPass = renderPass;
	createInfo.width = extent.width;
	createInfo.height = extent.height;
	createInfo.layers = layers;
	return m_device.createFramebuffer(createInfo);
}

void Device::defer(Deferred::Callback callback, u64 defer) {
	m_deferred.defer({std::move(callback), defer});
}

void Device::decrementDeferred() {
	m_deferred.decrement();
}
} // namespace le::graphics
