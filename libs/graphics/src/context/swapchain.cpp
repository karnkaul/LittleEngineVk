#include <map>
#include <core/maths.hpp>
#include <graphics/common.hpp>
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
	SwapchainCreateInfo(vk::PhysicalDevice pd, vk::SurfaceKHR surface, Swapchain::CreateInfo const& info) : pd(pd), surface(surface) {
		vk::SurfaceCapabilitiesKHR capabilities = pd.getSurfaceCapabilitiesKHR(surface);
		std::vector<vk::SurfaceFormatKHR> colourFormats = pd.getSurfaceFormatsKHR(surface);
		availableModes = pd.getSurfacePresentModesKHR(surface);
		std::map<u32, std::vector<vk::SurfaceFormatKHR>> ranked;
		for (auto const& available : colourFormats) {
			u32 spaceRank = 0;
			for (auto desired : info.desired.colourSpaces) {
				if (desired == available.colorSpace) {
					break;
				}
				++spaceRank;
			}
			u32 formatRank = 0;
			for (auto desired : info.desired.colourFormats) {
				if (desired == available.format) {
					break;
				}
				++formatRank;
			}
			ranked[spaceRank + formatRank].push_back(available);
		}
		colourFormat = ranked.begin()->second.front();
		for (auto format : info.desired.depthFormats) {
			vk::FormatProperties const props = pd.getFormatProperties(format);
			static constexpr auto features = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
			if ((props.optimalTilingFeatures & features) == features) {
				depthFormat = format;
				break;
			}
		}
		if (Device::default_v(depthFormat)) {
			depthFormat = vk::Format::eD16Unorm;
		}
		presentMode = bestFit(availableModes, info.desired.presentModes, availableModes.front());
		imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && capabilities.maxImageCount < imageCount) {
			imageCount = capabilities.maxImageCount;
		}
		if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque) {
			compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		} else if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) {
			compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eInherit;
		} else if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) {
			compositeAlpha = vk::CompositeAlphaFlagBitsKHR::ePreMultiplied;
		} else {
			compositeAlpha = vk::CompositeAlphaFlagBitsKHR::ePostMultiplied;
		}
	}

	vk::Extent2D extent(glm::ivec2 fbSize) {
		vk::SurfaceCapabilitiesKHR capabilities = pd.getSurfaceCapabilitiesKHR(surface);
		current.transform = capabilities.currentTransform;
		current.extent = capabilities.currentExtent;
		if (!Swapchain::valid(fbSize) || current.extent.width != maths::max<u32>()) {
			return capabilities.currentExtent;
		} else {
			return vk::Extent2D(std::clamp((u32)fbSize.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
								std::clamp((u32)fbSize.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height));
		}
	}

	vk::PhysicalDevice pd;
	vk::SurfaceKHR surface;
	std::vector<vk::PresentModeKHR> availableModes;
	vk::SurfaceFormatKHR colourFormat;
	vk::Format depthFormat = {};
	vk::PresentModeKHR presentMode = {};
	vk::CompositeAlphaFlagBitsKHR compositeAlpha;
	Swapchain::Display current;
	u32 imageCount = 0;
};
} // namespace

Swapchain::Frame& Swapchain::Storage::frame() {
	ENSURE(acquired, "Image not acquired");
	return frames[acquired->value];
}

Swapchain::Swapchain(VRAM& vram) : m_vram(vram), m_device(vram.m_device) {
	if (!m_device.get().valid(m_device.get().surface())) {
		throw std::runtime_error("Invalid surface");
	}
	m_metadata.surface = m_device.get().surface();
}

Swapchain::Swapchain(VRAM& vram, CreateInfo const& info, glm::ivec2 framebufferSize) : Swapchain(vram) {
	m_metadata.info = info;
	if (!construct(framebufferSize)) {
		throw std::runtime_error("Failed to construct Vulkan swapchain");
	}
	makeRenderPass();
	auto const extent = m_storage.current.extent;
	auto const mode = presentModeName(m_metadata.presentMode);
	g_log.log(lvl::info, 1, "[{}] Vulkan swapchain constructed [{}x{}] [{}]", g_name, extent.width, extent.height, mode);
}

Swapchain::~Swapchain() {
	if (!Device::default_v(m_storage.swapchain)) {
		g_log.log(lvl::info, 1, "[{}] Vulkan swapchain destroyed", g_name);
	}
	destroy(m_storage, true);
}

kt::result_t<RenderTarget> Swapchain::acquireNextImage(RenderSync const& sync) {
	orientCheck();
	if (m_storage.flags.any(Flag::ePaused | Flag::eOutOfDate)) {
		return {};
	}
	if (m_storage.acquired) {
		g_log.log(lvl::warning, 1, "[{}] Attempt to acquire image without presenting previously acquired one", g_name);
		return m_storage.frame().target;
	}
	try {
		m_storage.acquired = m_device.get().device().acquireNextImageKHR(m_storage.swapchain, maths::max<u64>(), sync.drawReady, {});
		setFlags(m_storage.acquired->result);
	} catch (vk::OutOfDateKHRError const& e) {
		m_storage.flags.set(Flag::eOutOfDate);
		g_log.log(lvl::warning, 1, "[{}] Swapchain failed to acquire next image [{}]", g_name, e.what());
		return {};
	}
	if (!m_storage.acquired || (m_storage.acquired->result != vk::Result::eSuccess && m_storage.acquired->result != vk::Result::eSuboptimalKHR)) {
		g_log.log(lvl::warning, 1, "[{}] Swapchain failed to acquire next image [{}]", g_name,
				  m_storage.acquired ? g_vkResultStr[m_storage.acquired->result] : "Unknown Error");
		m_storage.acquired.reset();
		return {};
	}
	auto& frame = m_storage.frame();
	m_device.get().waitFor(frame.drawn);
	return frame.target;
}

bool Swapchain::present(RenderSync const& sync) {
	if (m_storage.flags.any(Flag::ePaused | Flag::eOutOfDate)) {
		return false;
	}
	if (!m_storage.acquired) {
		g_log.log(lvl::warning, 1, "[{}] Attempt to present image without acquiring one", g_name);
		orientCheck();
		return false;
	}
	Frame& frame = m_storage.frame();
	vk::PresentInfoKHR presentInfo;
	auto const index = m_storage.acquired->value;
	presentInfo.waitSemaphoreCount = 1U;
	presentInfo.pWaitSemaphores = &sync.presentReady;
	presentInfo.swapchainCount = 1U;
	presentInfo.pSwapchains = &m_storage.swapchain;
	presentInfo.pImageIndices = &index;
	vk::Result result;
	try {
		result = m_device.get().queues().present(presentInfo, false);
	} catch (vk::OutOfDateKHRError const& e) {
		g_log.log(lvl::warning, 1, "[{}] Swapchain Failed to present image [{}]", g_name, e.what());
		m_storage.flags.set(Flag::eOutOfDate);
		m_storage.acquired.reset();
		return false;
	}
	setFlags(result);
	m_storage.acquired.reset();
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
		g_log.log(lvl::warning, 1, "[{}] Swapchain Failed to present image [{}]", g_name, g_vkResultStr[result]);
		return false;
	}
	frame.drawn = sync.drawing;
	orientCheck(); // Must submit acquired image, so skipping extent check here
	return true;
}

bool Swapchain::reconstruct(glm::ivec2 framebufferSize, Span<vk::PresentModeKHR> desiredModes) {
	if (!desiredModes.empty()) {
		m_metadata.info.desired.presentModes = desiredModes;
	}
	Storage retired = std::move(m_storage);
	m_metadata.retired = retired.swapchain;
	bool const bResult = construct(framebufferSize);
	auto const extent = m_storage.current.extent;
	auto const mode = presentModeName(m_metadata.presentMode);
	if (bResult) {
		g_log.log(lvl::info, 1, "[{}] Vulkan swapchain reconstructed [{}x{}] [{}]", g_name, extent.width, extent.height, mode);
	} else if (!m_storage.flags.test(Flag::ePaused)) {
		g_log.log(lvl::error, 1, "[{}] Vulkan swapchain reconstruction failed!", g_name);
	}
	destroy(retired, false);
	return bResult;
}

bool Swapchain::suboptimal() const noexcept {
	return m_storage.flags.test(Flag::eSuboptimal);
}

bool Swapchain::paused() const noexcept {
	return m_storage.flags.test(Flag::ePaused);
}

bool Swapchain::construct(glm::ivec2 framebufferSize) {
	m_storage = {};
	SwapchainCreateInfo info(m_device.get().physicalDevice().device, m_metadata.surface, m_metadata.info);
	m_metadata.availableModes = std::move(info.availableModes);
	{
		vk::SwapchainCreateInfoKHR createInfo;
		createInfo.minImageCount = info.imageCount;
		createInfo.imageFormat = info.colourFormat.format;
		createInfo.imageColorSpace = info.colourFormat.colorSpace;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		auto const indices = m_device.get().queues().familyIndices(QType::eGraphics | QType::ePresent);
		createInfo.imageSharingMode = indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
		createInfo.pQueueFamilyIndices = indices.data();
		createInfo.queueFamilyIndexCount = (u32)indices.size();
		createInfo.compositeAlpha = info.compositeAlpha;
		m_metadata.presentMode = createInfo.presentMode = info.presentMode;
		createInfo.clipped = vk::Bool32(true);
		createInfo.surface = m_metadata.surface;
		createInfo.oldSwapchain = m_metadata.retired;
		createInfo.imageExtent = info.extent(framebufferSize);
		createInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		if (createInfo.imageExtent.width <= 0 || createInfo.imageExtent.height <= 0) {
			m_storage.flags.set(Flag::ePaused);
			return false;
		}
		m_storage.current = info.current;
		m_storage.swapchain = m_device.get().device().createSwapchainKHR(createInfo);
		m_metadata.formats.colour = info.colourFormat;
		m_metadata.formats.depth = info.depthFormat;
		if (!m_metadata.original) {
			m_metadata.original = info.current;
		}
		m_metadata.retired = vk::SwapchainKHR();
	}
	{
		auto images = m_device.get().device().getSwapchainImagesKHR(m_storage.swapchain);
		ENSURE(images.size() < m_storage.frames.capacity(), "Too many swapchain images");
		Image::CreateInfo depthImageInfo;
		depthImageInfo.createInfo.format = info.depthFormat;
		depthImageInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		depthImageInfo.createInfo.extent = vk::Extent3D(m_storage.current.extent, 1);
		depthImageInfo.createInfo.tiling = vk::ImageTiling::eOptimal;
		depthImageInfo.createInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
#if !defined(LEVK_DESKTOP)
		depthImageInfo.preferred = vk::MemoryPropertyFlagBits::eLazilyAllocated;
		depthImageInfo.createInfo.usage |= vk::ImageUsageFlagBits::eTransientAttachment;
#endif
		depthImageInfo.createInfo.samples = vk::SampleCountFlagBits::e1;
		depthImageInfo.createInfo.imageType = vk::ImageType::e2D;
		depthImageInfo.createInfo.initialLayout = vk::ImageLayout::eUndefined;
		depthImageInfo.createInfo.mipLevels = 1;
		depthImageInfo.createInfo.arrayLayers = 1;
		depthImageInfo.queueFlags = QType::eGraphics;
		depthImageInfo.name = "swapchain_depth";
		m_storage.depthImage = Image(m_vram, depthImageInfo);
		m_storage.depthImageView = m_device.get().createImageView(m_storage.depthImage->image(), info.depthFormat, vk::ImageAspectFlagBits::eDepth);
		auto const format = info.colourFormat.format;
		auto const aspectFlags = vk::ImageAspectFlagBits::eColor;
		for (auto const& image : images) {
			Frame frame;
			frame.target.colour.image = image;
			frame.target.depth.image = m_storage.depthImage->image();
			frame.target.colour.view = m_device.get().createImageView(image, format, aspectFlags);
			frame.target.depth.view = m_storage.depthImageView;
			frame.target.extent = m_storage.current.extent;
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
		attachments[0].format = m_metadata.formats.colour.format;
		attachments[0].samples = vk::SampleCountFlagBits::e1;
		attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
		attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
		attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		// attachments[0].initialLayout = vk::ImageLayout::eUndefined;
		// attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;
		attachments[0].initialLayout = m_metadata.info.transitions.colour.first;
		attachments[0].finalLayout = m_metadata.info.transitions.colour.second;
		colourAttachment.attachment = 0;
		colourAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
	}
	{
		attachments[1].format = m_metadata.formats.depth;
		attachments[1].samples = vk::SampleCountFlagBits::e1;
		attachments[1].loadOp = vk::AttachmentLoadOp::eClear;
		attachments[1].storeOp = vk::AttachmentStoreOp::eDontCare;
		attachments[1].stencilLoadOp = vk::AttachmentLoadOp::eClear;
		attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		// attachments[1].initialLayout = vk::ImageLayout::eUndefined;
		// attachments[1].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		attachments[1].initialLayout = m_metadata.info.transitions.depth.first;
		attachments[1].finalLayout = m_metadata.info.transitions.depth.second;
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
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests;
	using AF = vk::AccessFlagBits;
	// dependency.dstAccessMask = AF::eColorAttachmentRead | AF::eColorAttachmentWrite | AF::eDepthStencilAttachmentRead | AF::eDepthStencilAttachmentWrite;
	// dependency.srcAccessMask = vk::AccessFlagBits::eMemoryWrite;
	// dependency.dstAccessMask = vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite;
	// dependency.srcStageMask = vk::PipelineStageFlagBits::eAllGraphics;
	// dependency.dstStageMask = vk::PipelineStageFlagBits::eAllGraphics;

	// dependency.srcAccessMask = AF::eColorAttachmentWrite;
	dependency.dstAccessMask = AF::eColorAttachmentWrite | AF::eDepthStencilAttachmentWrite;
	m_metadata.renderPass = m_device.get().createRenderPass(attachments, subpass, dependency);
}

void Swapchain::destroy(Storage& out_storage, bool bMeta) {
	auto r = bMeta ? std::exchange(m_metadata.renderPass, vk::RenderPass()) : vk::RenderPass();
	m_device.get().waitIdle();
	auto lock = m_device.get().queues().lock();
	for (auto& frame : out_storage.frames) {
		m_device.get().destroy(frame.target.colour.view);
	}
	m_device.get().destroy(out_storage.depthImageView, out_storage.swapchain, r);
	out_storage = {};
}

void Swapchain::setFlags(vk::Result result) {
	switch (result) {
	case vk::Result::eSuboptimalKHR: {
		if (!m_storage.flags.test(Swapchain::Flag::eSuboptimal)) {
			g_log.log(lvl::debug, 2, "[{}] Vulkan swapchain is suboptimal", g_name);
		}
		m_storage.flags.set(Swapchain::Flag::eSuboptimal);
		break;
	}
	case vk::Result::eErrorOutOfDateKHR: {
		if (!m_storage.flags.test(Swapchain::Flag::eOutOfDate)) {
			g_log.log(lvl::debug, 2, "[{}] Vulkan swapchain is out of date", g_name);
		}
		m_storage.flags.set(Swapchain::Flag::eOutOfDate);
		break;
	}
	default:
		break;
	}
}

void Swapchain::orientCheck() {
	auto const capabilities = m_device.get().physicalDevice().surfaceCapabilities(m_metadata.surface);
	if (capabilities.currentExtent != maths::max<u32>() && capabilities.currentExtent != m_storage.current.extent) {
		m_storage.flags.set(Flag::eOutOfDate);
	}
}
} // namespace le::graphics
