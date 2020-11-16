#include <map>
#include <core/maths.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/swapchain.hpp>
#include <graphics/context/vram.hpp>

namespace le::graphics {
namespace {
template <typename T, typename U, typename V>
constexpr T bestFit(U&& all, V&& desired, T fallback) noexcept {
	for (auto const& d : desired) {
		if (std::find(all.begin(), all.end(), d) != all.end()) {
			return d;
		}
	}
	return fallback;
}

struct SwapchainCreateInfo {
	SwapchainCreateInfo(vk::PhysicalDevice pd, vk::SurfaceKHR surface, Swapchain::CreateInfo const& options) : pd(pd), surface(surface) {
		vk::SurfaceCapabilitiesKHR capabilities = pd.getSurfaceCapabilitiesKHR(surface);
		std::vector<vk::SurfaceFormatKHR> colourFormats = pd.getSurfaceFormatsKHR(surface);
		availableModes = pd.getSurfacePresentModesKHR(surface);
		std::map<u32, vk::SurfaceFormatKHR> ranked;
		for (auto const& available : colourFormats) {
			u32 spaceRank = 0;
			for (auto desired : options.desired.colourSpaces) {
				if (desired == available.colorSpace) {
					break;
				}
				++spaceRank;
			}
			u32 formatRank = 0;
			for (auto desired : options.desired.colourFormats) {
				if (desired == available.format) {
					break;
				}
				++formatRank;
			}
			ranked.emplace(spaceRank + formatRank, available);
		}
		colourFormat = ranked.begin()->second;
		for (auto format : options.desired.depthFormats) {
			vk::FormatProperties const props = pd.getFormatProperties(format);
			static constexpr auto features = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
			if ((props.optimalTilingFeatures & features) == features) {
				depthFormat = format;
				break;
			}
		}
		if (default_v(depthFormat)) {
			depthFormat = vk::Format::eD16Unorm;
		}
		presentMode = bestFit(availableModes, options.desired.presentModes, availableModes.front());
		transform = capabilities.currentTransform;
		imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && capabilities.maxImageCount < imageCount) {
			imageCount = capabilities.maxImageCount;
		}
	}

	vk::Extent2D extent(glm::ivec2 fbSize) const {
		vk::Extent2D extent;
		vk::SurfaceCapabilitiesKHR capabilities = pd.getSurfaceCapabilitiesKHR(surface);
		if (capabilities.currentExtent.width != maths::max<u32>()) {
			extent = capabilities.currentExtent;
		} else {
			extent = vk::Extent2D(std::clamp((u32)fbSize.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
								  std::clamp((u32)fbSize.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height));
		}
		ENSURE(extent.width > 0 && extent.height > 0, "Invariant violated");
		return extent;
	}

	vk::PhysicalDevice pd;
	vk::SurfaceKHR surface;
	std::vector<vk::PresentModeKHR> availableModes;
	vk::SurfaceFormatKHR colourFormat;
	vk::Format depthFormat = {};
	vk::PresentModeKHR presentMode = {};
	vk::SurfaceTransformFlagBitsKHR transform;
	u32 imageCount = 0;
};

void setFlags(Swapchain::Flags& out_flags, vk::Result result) {
	switch (result) {
	case vk::Result::eSuboptimalKHR:
		out_flags.set(Swapchain::Flag::eSuboptimal);
		break;
	case vk::Result::eErrorOutOfDateKHR:
		out_flags.set(Swapchain::Flag::eOutOfDate);
		break;
	default:
		break;
	}
}
} // namespace

Swapchain::Frame& Swapchain::Storage::frame() {
	return frames[imageIndex];
}

Swapchain::Swapchain(VRAM& vram) : m_vram(vram), m_device(vram.m_device) {
	Device& d = vram.m_device;
	if (!d.valid(d.m_metadata.surface)) {
		throw std::runtime_error("Invalid surface");
	}
	m_metadata.surface = d.m_metadata.surface;
}

Swapchain::Swapchain(VRAM& vram, glm::ivec2 framebufferSize, CreateInfo const& info) : Swapchain(vram) {
	m_metadata.info = info;
	if (!construct(framebufferSize)) {
		throw std::runtime_error("Failed to construct Vulkan swapchain");
	}
	makeRenderPass();
	logD("[{}] Vulkan swapchain constructed", g_name);
}

Swapchain::~Swapchain() {
	logD_if(!default_v(m_storage.swapchain), "[{}] Vulkan swapchain destroyed", g_name);
	destroy(true);
}

std::optional<RenderTarget> Swapchain::acquireNextImage(vk::Semaphore setDrawReady) {
	if (m_storage.flags.any(Flag::ePaused | Flag::eOutOfDate)) {
		return std::nullopt;
	}
	Device& d = static_cast<VRAM&>(m_vram).m_device;
	std::optional<vk::ResultValue<u32>> acquire;
	try {
		acquire = d.m_device.acquireNextImageKHR(m_storage.swapchain, maths::max<u64>(), setDrawReady, {});
		setFlags(m_storage.flags, acquire->result);
	} catch (vk::OutOfDateKHRError const& e) {
		m_storage.flags.set(Flag::eOutOfDate);
		logD("[{}] Swapchain failed to acquire next image [{}]", g_name, e.what());
		return std::nullopt;
	}
	if (!acquire || (acquire->result != vk::Result::eSuccess && acquire->result != vk::Result::eSuboptimalKHR)) {
		logD("[{}] Swapchain failed to acquire next image [{}]", g_name, acquire ? g_vkResultStr[acquire->result] : "Unknown Error");
		return std::nullopt;
	}
	m_storage.imageIndex = (u32)acquire->value;
	auto& frame = m_storage.frame();
	d.waitFor(frame.drawn);
	return frame.target;
}

bool Swapchain::present(vk::Semaphore drawWait, vk::Fence onDrawn) {
	if (m_storage.flags.any(Flag::ePaused | Flag::eOutOfDate)) {
		return false;
	}
	Device& d = static_cast<VRAM&>(m_vram).m_device;
	Frame& frame = m_storage.frame();
	vk::PresentInfoKHR presentInfo;
	auto const index = m_storage.imageIndex;
	presentInfo.waitSemaphoreCount = 1U;
	presentInfo.pWaitSemaphores = &drawWait;
	presentInfo.swapchainCount = 1U;
	presentInfo.pSwapchains = &m_storage.swapchain;
	presentInfo.pImageIndices = &index;
	vk::Result result;
	try {
		result = d.m_queues.present(presentInfo);
	} catch (vk::OutOfDateKHRError const& e) {
		logD("[{}] Swapchain Failed to present image [{}]", g_name, e.what());
		m_storage.flags.set(Flag::eOutOfDate);
		return false;
	}
	setFlags(m_storage.flags, result);
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
		logD("[{}] Swapchain Failed to present image [{}]", g_name, g_vkResultStr[result]);
		return false;
	}
	frame.drawn = onDrawn;
	return true;
}

bool Swapchain::reconstruct(glm::ivec2 framebufferSize, Span<vk::PresentModeKHR> desiredModes) {
	destroy(false);
	if (!desiredModes.empty()) {
		m_metadata.info.desired.presentModes = desiredModes;
	}
	if (construct(framebufferSize)) {
		logD("[{}] Vulkan swapchain reconstructed", g_name);
		return true;
	}
	logW_if(!m_storage.flags.test(Flag::ePaused), "[{}] Vulkan swapchain reconstruction failed!", g_name);
	return false;
}

Swapchain::Flags Swapchain::flags() const noexcept {
	return m_storage.flags;
}

bool Swapchain::suboptimal() const noexcept {
	return m_storage.flags.test(Flag::eSuboptimal);
}

bool Swapchain::paused() const noexcept {
	return m_storage.flags.test(Flag::ePaused);
}

vk::RenderPass Swapchain::renderPass() const noexcept {
	return m_metadata.renderPass;
}

bool Swapchain::construct(glm::ivec2 framebufferSize) {
	if (!valid(framebufferSize)) {
		m_storage.flags.set(Flag::ePaused);
		return false;
	}
	VRAM& v = m_vram;
	Device& d = v.m_device;
	m_storage = {};
	SwapchainCreateInfo info(d.m_physicalDevice, m_metadata.surface, m_metadata.info);
	m_metadata.availableModes = std::move(info.availableModes);
	{
		vk::SwapchainCreateInfoKHR createInfo;
		createInfo.minImageCount = info.imageCount;
		createInfo.imageFormat = info.colourFormat.format;
		createInfo.imageColorSpace = info.colourFormat.colorSpace;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		auto const indices = d.m_queues.familyIndices(QType::eGraphics | QType::ePresent);
		createInfo.imageSharingMode = indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
		createInfo.pQueueFamilyIndices = indices.data();
		createInfo.queueFamilyIndexCount = (u32)indices.size();
		createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		createInfo.presentMode = info.presentMode;
		createInfo.clipped = vk::Bool32(true);
		createInfo.surface = m_metadata.surface;
		createInfo.preTransform = info.transform;
		m_storage.extent = createInfo.imageExtent = info.extent(framebufferSize);
		m_storage.swapchain = d.m_device.createSwapchainKHR(createInfo);
		m_metadata.formats.colour = info.colourFormat.format;
		m_metadata.formats.depth = info.depthFormat;
	}
	{
		auto images = d.m_device.getSwapchainImagesKHR(m_storage.swapchain);
		m_storage.frames.reserve(images.size());
		Image::CreateInfo depthImageInfo;
		depthImageInfo.createInfo.format = info.depthFormat;
		depthImageInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		depthImageInfo.createInfo.extent = vk::Extent3D(m_storage.extent, 1);
		depthImageInfo.createInfo.tiling = vk::ImageTiling::eOptimal;
		depthImageInfo.createInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
		depthImageInfo.createInfo.samples = vk::SampleCountFlagBits::e1;
		depthImageInfo.createInfo.imageType = vk::ImageType::e2D;
		depthImageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		depthImageInfo.createInfo.mipLevels = 1;
		depthImageInfo.createInfo.arrayLayers = 1;
		depthImageInfo.queueFlags = QType::eGraphics;
		depthImageInfo.name = "swapchain_depth";
		m_storage.depthImage = v.construct(depthImageInfo);
		m_storage.depthImageView = d.createImageView(m_storage.depthImage.image, info.depthFormat, vk::ImageAspectFlagBits::eDepth);
		auto const format = info.colourFormat.format;
		auto const aspectFlags = vk::ImageAspectFlagBits::eColor;
		for (auto const& image : images) {
			Frame frame;
			frame.target.colour.image = image;
			frame.target.depth.image = m_storage.depthImage.image;
			frame.target.colour.view = d.createImageView(image, format, aspectFlags);
			frame.target.depth.view = m_storage.depthImageView;
			frame.target.extent = m_storage.extent;
			ENSURE(frame.target.extent.width > 0 && frame.target.extent.height > 0, "Invariant violated");
			m_storage.frames.push_back(std::move(frame));
		}
		if (m_storage.frames.empty()) {
			throw std::runtime_error("Failed to construct Vulkan swapchain!");
		}
	}
	return true;
}

void Swapchain::makeRenderPass() {
	std::array<vk::AttachmentDescription, 2> attachments;
	vk::AttachmentReference colourAttachment, depthAttachment;
	{
		attachments[0].format = m_metadata.formats.colour;
		attachments[0].samples = vk::SampleCountFlagBits::e1;
		attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
		attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachments[0].initialLayout = vk::ImageLayout::eUndefined;
		attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;
		colourAttachment.attachment = 0;
		colourAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
	}
	{
		attachments[1].format = m_metadata.formats.depth;
		attachments[1].samples = vk::SampleCountFlagBits::e1;
		attachments[1].loadOp = vk::AttachmentLoadOp::eClear;
		attachments[1].storeOp = vk::AttachmentStoreOp::eDontCare;
		attachments[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachments[1].initialLayout = vk::ImageLayout::eUndefined;
		attachments[1].finalLayout = vk::ImageLayout::ePresentSrcKHR;
		depthAttachment.attachment = 1;
		depthAttachment.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	}
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachment;
	subpass.pDepthStencilAttachment = &depthAttachment;
	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
	Device& d = m_device;
	m_metadata.renderPass = d.createRenderPass(attachments, subpass, dependency);
}

void Swapchain::destroy(bool bMeta) {
	VRAM& v = m_vram;
	Device& d = v.m_device;
	auto r = bMeta ? std::exchange(m_metadata.renderPass, vk::RenderPass()) : vk::RenderPass();
	d.m_queues.waitIdle(QType::eGraphics);
	for (auto& frame : m_storage.frames) {
		d.destroy(frame.target.colour.view);
	}
	d.destroy(m_storage.depthImageView, m_storage.swapchain, r);
	v.destroy(m_storage.depthImage);
	m_storage = {};
}
} // namespace le::graphics
