#include <map>
#include <core/maths.hpp>
#include <graphics/common.hpp>
#include <graphics/context/device.hpp>
#include <graphics/context/vram.hpp>
#include <graphics/render/swapchain.hpp>

namespace le::graphics {
vk::SurfaceFormatKHR Swapchain::FormatPicker::pick(Span<vk::SurfaceFormatKHR const> options) const noexcept {
	static constexpr auto space = vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear;
	static constexpr std::array formats = {vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Srgb};
	std::vector<vk::SurfaceFormatKHR> avail;
	avail.reserve(options.size());
	for (auto const& option : options) {
		if (option.colorSpace == space && std::find(formats.begin(), formats.end(), option.format) != formats.end()) { avail.push_back(option); }
	}
	for (auto format : formats) {
		for (auto const& option : avail) {
			if (option.format == format) { return option; }
		}
	}
	return options.front();
}

namespace {
template <typename T, typename U, typename V>
constexpr T bestFit(U&& all, V&& desired, T fallback) noexcept {
	for (auto const& d : desired) {
		if (std::find(all.begin(), all.end(), d) != all.end()) { return d; }
	}
	return fallback;
}

struct SwapchainCreateInfo {
	SwapchainCreateInfo(vk::PhysicalDevice pd, vk::SurfaceKHR surface, Swapchain::CreateInfo const& info) : pd(pd), surface(surface) {
		usage = vk::ImageUsageFlagBits::eColorAttachment;
		if (info.transfer) { usage |= vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc; }
		vk::SurfaceCapabilitiesKHR capabilities = pd.getSurfaceCapabilitiesKHR(surface);
		std::vector<vk::SurfaceFormatKHR> colourFormats = pd.getSurfaceFormatsKHR(surface);
		availableModes = pd.getSurfacePresentModesKHR(surface);
		static Swapchain::FormatPicker const s_picker;
		Swapchain::FormatPicker const* picker = info.custom ? info.custom : &s_picker;
		colourFormat = picker->pick(colourFormats);
		static constexpr std::array depthFormats = {vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint};
		for (auto format : depthFormats) {
			vk::FormatProperties const props = pd.getFormatProperties(format);
			static constexpr auto features = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
			if ((props.optimalTilingFeatures & features) == features) {
				depthFormat = format;
				break;
			}
		}
		if (Device::default_v(depthFormat)) { depthFormat = vk::Format::eD16Unorm; }
		kt::fixed_vector<vk::PresentModeKHR, 8> presentModes;
		if (info.vsync || Swapchain::s_forceVsync) { presentModes.push_back(vk::PresentModeKHR::eImmediate); }
		if constexpr (levk_desktopOS) { presentModes.push_back(vk::PresentModeKHR::eMailbox); }
		presentModes.push_back(vk::PresentModeKHR::eFifoRelaxed);
		presentModes.push_back(vk::PresentModeKHR::eFifo);
		presentMode = bestFit(availableModes, presentModes, availableModes.front());
		if (info.vsync || Swapchain::s_forceVsync) {
			static auto const name = presentModeNames[vk::PresentModeKHR::eImmediate];
			if (presentMode == vk::PresentModeKHR::eImmediate) {
				g_log.log(lvl::info, 0, "[{}] VSYNC ({} present mode) requested", g_name, name);
			} else {
				g_log.log(lvl::warning, 0, "[{}] VSYNC ({} present mode) requested but not available!", g_name, name);
			}
		}
		imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && capabilities.maxImageCount < imageCount) { imageCount = capabilities.maxImageCount; }
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

	Extent2D extent(glm::ivec2 fbSize) {
		vk::SurfaceCapabilitiesKHR capabilities = pd.getSurfaceCapabilitiesKHR(surface);
		current.transform = capabilities.currentTransform;
		if (!Swapchain::valid(fbSize) || current.extent.x != maths::max<u32>()) {
			current.extent = cast(capabilities.currentExtent);
		} else {
			current.extent = {std::clamp((u32)fbSize.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
							  std::clamp((u32)fbSize.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
		}
		return current.extent;
	}

	vk::PhysicalDevice pd;
	vk::SurfaceKHR surface;
	std::vector<vk::PresentModeKHR> availableModes;
	vk::SurfaceFormatKHR colourFormat;
	vk::Format depthFormat = {};
	vk::PresentModeKHR presentMode = {};
	vk::CompositeAlphaFlagBitsKHR compositeAlpha;
	vk::ImageUsageFlags usage;
	Swapchain::Display current;
	u32 imageCount = 0;
};
} // namespace

Swapchain::Acquire Swapchain::Storage::current() const {
	ensure(acquired.has_value(), "Image not acquired");
	return {images[(std::size_t)*acquired], *acquired};
}

Swapchain::Acquire Swapchain::Storage::current(u32 acquired) {
	this->acquired = acquired;
	return {images[(std::size_t)acquired], acquired};
}

Swapchain::Swapchain(not_null<VRAM*> vram) : m_vram(vram), m_device(vram->m_device) {
	if (!m_device->valid(m_device->surface())) { throw std::runtime_error("Invalid surface"); }
	m_metadata.surface = m_device->surface();
}

Swapchain::Swapchain(not_null<VRAM*> vram, CreateInfo const& info, glm::ivec2 framebufferSize) : Swapchain(vram) {
	m_metadata.info = info;
	if (!construct(framebufferSize)) { throw std::runtime_error("Failed to construct Vulkan swapchain"); }
	auto const extent = m_storage.display.extent;
	auto const mode = presentModeNames[m_metadata.presentMode];
	g_log.log(lvl::info, 1, "[{}] Vulkan swapchain constructed [{}x{}] [{}]", g_name, extent.x, extent.y, mode);
}

Swapchain::~Swapchain() {
	m_vram->shutdown(); // stop transfer polling
	if (!Device::default_v(m_storage.swapchain)) { g_log.log(lvl::info, 1, "[{}] Vulkan swapchain destroyed", g_name); }
	destroy(m_storage);
}

kt::result<Swapchain::Acquire> Swapchain::acquireNextImage(vk::Semaphore ssignal, vk::Fence fsignal) {
	orientCheck();
	if (m_storage.flags.any(Flags(Flag::ePaused) | Flag::eOutOfDate)) { return kt::null_result; }
	if (m_storage.acquired) {
		g_log.log(lvl::warning, 1, "[{}] Attempt to acquire image without presenting previously acquired one", g_name);
		return m_storage.current();
	}
	u32 acquired;
	auto const result = m_device->device().acquireNextImageKHR(m_storage.swapchain, maths::max<u64>(), ssignal, fsignal, &acquired);
	setFlags(result);
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
		g_log.log(lvl::warning, 1, "[{}] Swapchain failed to acquire next image [{}]", g_name, g_vkResultStr[result]);
		return kt::null_result;
	}
	return m_storage.current(acquired);
}

bool Swapchain::present(vk::Semaphore swait) {
	if (m_storage.flags.any(Flags(Flag::ePaused) | Flag::eOutOfDate)) { return false; }
	if (!m_storage.acquired) {
		g_log.log(lvl::warning, 1, "[{}] Attempt to present image without acquiring one", g_name);
		orientCheck();
		return false;
	}
	vk::PresentInfoKHR presentInfo;
	u32 const index = *m_storage.acquired;
	presentInfo.waitSemaphoreCount = 1U;
	presentInfo.pWaitSemaphores = &swait;
	presentInfo.swapchainCount = 1U;
	presentInfo.pSwapchains = &m_storage.swapchain;
	presentInfo.pImageIndices = &index;
	auto const result = m_device->queues().present(presentInfo, false);
	setFlags(result);
	m_storage.acquired.reset();
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
		g_log.log(lvl::warning, 1, "[{}] Swapchain Failed to present image [{}]", g_name, g_vkResultStr[result]);
		return false;
	}
	orientCheck(); // Must submit acquired image, so skipping extent check here
	return true;
}

bool Swapchain::reconstruct(glm::ivec2 framebufferSize, bool vsync) {
	m_metadata.info.vsync = vsync;
	Storage retired = std::move(m_storage);
	m_metadata.retired = retired.swapchain;
	bool const bResult = construct(framebufferSize);
	auto const extent = m_storage.display.extent;
	auto const mode = presentModeNames[m_metadata.presentMode];
	if (bResult) {
		g_log.log(lvl::info, 1, "[{}] Vulkan swapchain reconstructed [{}x{}] [{}]", g_name, extent.x, extent.y, mode);
	} else if (!m_storage.flags.test(Flag::ePaused)) {
		g_log.log(lvl::error, 1, "[{}] Vulkan swapchain reconstruction failed!", g_name);
	}
	destroy(retired);
	return bResult;
}

bool Swapchain::suboptimal() const noexcept { return m_storage.flags.test(Flag::eSuboptimal); }

bool Swapchain::paused() const noexcept { return m_storage.flags.test(Flag::ePaused); }

bool Swapchain::construct(glm::ivec2 framebufferSize) {
	m_storage = {};
	SwapchainCreateInfo info(m_device->physicalDevice().device, m_metadata.surface, m_metadata.info);
	if (info.colourFormat.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear && !srgb(info.colourFormat.format)) {
		g_log.log(lvl::warning, 0,
				  "[{}] Swapchain image format is not sRGB! If linear (Unorm), Vulkan will not gamma correct writes to it, "
				  "and interpolation, blending, and lighting *will be* incorrect!",
				  g_name);
	}
	m_metadata.availableModes = std::move(info.availableModes);
	{
		vk::SwapchainCreateInfoKHR createInfo;
		createInfo.minImageCount = info.imageCount;
		createInfo.imageFormat = info.colourFormat.format;
		createInfo.imageColorSpace = info.colourFormat.colorSpace;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | info.usage;
		auto const indices = m_device->queues().familyIndices(QFlags(QType::eGraphics) | QType::ePresent);
		createInfo.imageSharingMode = indices.size() == 1 ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent;
		createInfo.pQueueFamilyIndices = indices.data();
		createInfo.queueFamilyIndexCount = (u32)indices.size();
		createInfo.compositeAlpha = info.compositeAlpha;
		m_metadata.presentMode = createInfo.presentMode = info.presentMode;
		createInfo.clipped = vk::Bool32(true);
		createInfo.surface = m_metadata.surface;
		createInfo.oldSwapchain = m_metadata.retired;
		createInfo.imageExtent = cast(info.extent(framebufferSize));
		createInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		if (createInfo.imageExtent.width <= 0 || createInfo.imageExtent.height <= 0) {
			m_storage.flags.set(Flag::ePaused);
			return false;
		}
		auto const result = m_device->device().createSwapchainKHR(&createInfo, nullptr, &m_storage.swapchain);
		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) { return false; }
		m_storage.display = info.current;
		m_metadata.formats.colour = info.colourFormat;
		m_metadata.formats.depth = info.depthFormat;
		if (!m_metadata.original) { m_metadata.original = info.current; }
		m_metadata.retired = vk::SwapchainKHR();
	}
	{
		auto images = m_device->device().getSwapchainImagesKHR(m_storage.swapchain);
		ensure(images.size() < m_storage.images.capacity(), "Too many swapchain images");
		auto const format = info.colourFormat.format;
		auto const aspectFlags = vk::ImageAspectFlagBits::eColor;
		for (auto const& image : images) { m_storage.images.push_back({image, m_device->makeImageView(image, format, aspectFlags), m_storage.display.extent}); }
		if (m_storage.images.empty()) { throw std::runtime_error("Failed to construct Vulkan swapchain!"); }
	}
	return true;
}

void Swapchain::destroy(Storage& out_storage) {
	m_device->waitIdle();
	auto lock = m_device->queues().lock();
	for (auto& image : out_storage.images) { m_device->destroy(image.view); }
	m_device->destroy(out_storage.swapchain);
	out_storage = {};
}

void Swapchain::setFlags(vk::Result result) {
	switch (result) {
	case vk::Result::eSuboptimalKHR: {
		if (!m_storage.flags.test(Swapchain::Flag::eSuboptimal)) { g_log.log(lvl::debug, 2, "[{}] Vulkan swapchain is suboptimal", g_name); }
		m_storage.flags.set(Swapchain::Flag::eSuboptimal);
		break;
	}
	case vk::Result::eErrorOutOfDateKHR: {
		if (!m_storage.flags.test(Swapchain::Flag::eOutOfDate)) { g_log.log(lvl::debug, 2, "[{}] Vulkan swapchain is out of date", g_name); }
		m_storage.flags.set(Swapchain::Flag::eOutOfDate);
		break;
	}
	default: break;
	}
}

void Swapchain::orientCheck() {
	auto const capabilities = m_device->physicalDevice().surfaceCapabilities(m_metadata.surface);
	if (capabilities.currentExtent != maths::max<u32>() && capabilities.currentExtent != cast(m_storage.display.extent)) {
		m_storage.flags.set(Flag::eOutOfDate);
	}
}
} // namespace le::graphics
