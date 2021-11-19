#include <core/maths.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/surface.hpp>

namespace le::graphics::foo {
namespace {
constexpr vk::Extent2D clamp(Extent2D target, vk::Extent2D lo, vk::Extent2D hi) noexcept {
	auto const x = std::clamp(target.x, lo.width, hi.width);
	auto const y = std::clamp(target.y, lo.height, hi.height);
	return {x, y};
}

vk::SurfaceFormatKHR bestColour(vk::PhysicalDevice pd, vk::SurfaceKHR surface) {
	auto const formats = pd.getSurfaceFormatsKHR(surface);
	for (auto const& format : formats) {
		if (format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
			if (format.format == vk::Format::eR8G8B8A8Unorm || format.format == vk::Format::eB8G8R8A8Unorm) { return format; }
		}
	}
	return formats.empty() ? vk::SurfaceFormatKHR() : formats.front();
}

vk::Format bestDepth(vk::PhysicalDevice pd) {
	static constexpr vk::Format formats[] = {vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint};
	static constexpr auto features = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
	for (auto const format : formats) {
		vk::FormatProperties const props = pd.getFormatProperties(format);
		if ((props.optimalTilingFeatures & features) == features) { return format; }
	}
	return vk::Format::eD16Unorm;
}

Surface::VSyncs availableVsyncs(vk::PhysicalDevice pd, vk::SurfaceKHR surface) {
	Surface::VSyncs ret;
	for (auto const mode : pd.getSurfacePresentModesKHR(surface)) {
		switch (mode) {
		case vk::PresentModeKHR::eFifo: ret.set(VSync::eOn); break;
		case vk::PresentModeKHR::eFifoRelaxed: ret.set(VSync::eAdaptive); break;
		case vk::PresentModeKHR::eImmediate: ret.set(VSync::eOff); break;
		default: break;
		}
	}
	return ret;
}

constexpr VSync bestVSync(Surface::VSyncs vsyncs, std::optional<VSync> force) noexcept {
	if (force && vsyncs.test(*force)) { return *force; }
	return vsyncs.test(VSync::eAdaptive) ? VSync::eAdaptive : VSync::eOn;
}

constexpr EnumArray<VSync, vk::PresentModeKHR> g_modes = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eFifoRelaxed, vk::PresentModeKHR::eImmediate};

vk::UniqueImageView makeImageView(vk::Device device, vk::Image image, vk::Format format) {
	vk::ImageViewCreateInfo info;
	info.viewType = vk::ImageViewType::e2D;
	info.format = format;
	info.components.r = vk::ComponentSwizzle::eR;
	info.components.g = vk::ComponentSwizzle::eG;
	info.components.b = vk::ComponentSwizzle::eB;
	info.components.a = vk::ComponentSwizzle::eA;
	info.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
	info.image = image;
	return device.createImageViewUnique(info);
}
} // namespace

std::optional<Swapchain::Acquire> Swapchain::acquireNextImage(vk::Semaphore signal) {
	std::uint32_t idx{};
	auto const result = device.acquireNextImageKHR(swapchain, maths::max<u64>(), signal, {}, &idx);
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) { return std::nullopt; }
	auto const i = std::size_t(idx);
	assert(i < images.size());
	return Acquire{images[i], idx};
}

vk::Result Swapchain::present(Acquire image, vk::Semaphore wait) {
	vk::PresentInfoKHR info;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &wait;
	info.swapchainCount = 1;
	info.pSwapchains = &swapchain;
	info.pImageIndices = &image.index;
	return queue.presentKHR(&info);
}

Surface::Surface(not_null<VRAM*> vram) noexcept : m_surface(vram->m_device->makeSurface()), m_vram(vram) {}

Surface::~Surface() { m_vram->m_device->waitIdle(); }

bool Surface::makeSwapchain(Extent2D fbSize, std::optional<VSync> vsync) {
	m_retired = std::move(m_storage);
	m_storage = {};
	auto const pd = m_vram->m_device->physicalDevice().device;
	m_vsyncs = availableVsyncs(pd, *m_surface);
	m_storage.format.vsync = bestVSync(m_vsyncs, vsync);
	m_createInfo.oldSwapchain = m_retired.swapchain ? *m_retired.swapchain : vk::SwapchainKHR();
	m_createInfo.presentMode = g_modes[m_storage.format.vsync];
	m_createInfo.imageArrayLayers = 1U;
	m_createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	m_createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	auto const queues = m_vram->m_device->queues().familyIndices(QFlags(QType::eGraphics, QType::ePresent));
	m_createInfo.pQueueFamilyIndices = queues.data();
	m_createInfo.queueFamilyIndexCount = (u32)queues.size();
	m_createInfo.surface = *m_surface;
	m_storage.format.colour = bestColour(pd, *m_surface);
	m_storage.format.depth = bestDepth(pd);
	m_createInfo.imageFormat = m_storage.format.colour.format;
	m_createInfo.imageColorSpace = m_storage.format.colour.colorSpace;
	// m_vram->m_device->waitIdle();
	m_storage.info = makeInfo(fbSize);
	if (m_storage.info.extent.width == 0 || m_storage.info.extent.height == 0) { return false; }
	m_createInfo.imageExtent = m_storage.info.extent;
	m_createInfo.minImageCount = m_storage.info.imageCount;
	vk::SwapchainKHR swapchain;
	auto const ret = m_vram->m_device->device().createSwapchainKHR(&m_createInfo, nullptr, &swapchain) == vk::Result::eSuccess;
	auto const extent = cast(m_storage.info.extent);
	if (ret) {
		m_storage.swapchain = vk::UniqueSwapchainKHR(swapchain, m_vram->m_device->device());
		m_swapchain.swapchain = *m_storage.swapchain;
		auto const images = m_vram->m_device->device().getSwapchainImagesKHR(m_swapchain.swapchain);
		for (auto const image : images) {
			m_storage.imageViews.push_back(makeImageView(m_vram->m_device->device(), image, m_createInfo.imageFormat));
			m_storage.images.push_back({image, *m_storage.imageViews.back(), extent});
		}
	}
	return ret;
}

std::optional<Surface::Acquire> Surface::acquireNextImage(Extent2D fbSize, vk::Semaphore signal) {
	if (m_storage.swapchain) {
		if (auto acquire = swapchain().acquireNextImage(signal)) {
			return acquire;
		} else {
			makeSwapchain(fbSize, format().vsync);
		}
	}
	return std::nullopt;
}

void Surface::submit(Span<vk::CommandBuffer const> cbs, Sync const& sync) const {
	static constexpr vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eTopOfPipe;
	vk::SubmitInfo submitInfo;
	submitInfo.pWaitDstStageMask = &waitStages;
	submitInfo.commandBufferCount = (u32)cbs.size();
	submitInfo.pCommandBuffers = cbs.data();
	submitInfo.waitSemaphoreCount = 1U;
	submitInfo.pWaitSemaphores = &sync.wait;
	submitInfo.signalSemaphoreCount = 1U;
	submitInfo.pSignalSemaphores = &sync.ssignal;
	m_vram->m_device->queues().submit(graphics::QType::eGraphics, submitInfo, sync.fsignal, true);
}

bool Surface::present(Extent2D fbSize, Acquire image, vk::Semaphore wait) {
	if (m_storage.swapchain) {
		auto const result = swapchain().present(image, wait);
		bool const ret = result == vk::Result::eSuccess;
		if (!ret) {
			makeSwapchain(fbSize, format().vsync);
			return false;
		}
		return ret;
	}
	return false;
}

Swapchain Surface::swapchain() const noexcept {
	auto ret = m_swapchain;
	ret.images = m_storage.images;
	ret.device = m_vram->m_device->device();
	ret.queue = m_vram->m_device->queues().queue(QType::ePresent).queue;
	return ret;
}

Surface::Info Surface::makeInfo(Extent2D extent) const {
	Info ret;
	auto const caps = m_vram->m_device->physicalDevice().device.getSurfaceCapabilitiesKHR(*m_surface);
	if (caps.currentExtent.width < maths::max<u32>() && caps.currentExtent.height < maths::max<u32>()) {
		ret.extent = caps.currentExtent;
	} else {
		ret.extent = clamp(extent, caps.minImageExtent, std::max(caps.maxImageExtent, caps.minImageExtent));
	}
	if (caps.maxImageCount == 0) {
		ret.imageCount = std::min(3U, caps.minImageCount);
	} else {
		ret.imageCount = std::clamp(3U, caps.minImageCount, caps.maxImageCount);
	}
	return ret;
}
} // namespace le::graphics::foo
