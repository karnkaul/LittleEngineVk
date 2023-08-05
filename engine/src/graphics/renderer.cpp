#include <le/core/logger.hpp>
#include <le/graphics/device.hpp>
#include <le/graphics/image_barrier.hpp>
#include <le/graphics/renderer.hpp>

namespace le::graphics {
namespace {
constexpr auto image_count(vk::SurfaceCapabilitiesKHR const& caps) noexcept -> std::uint32_t {
	if (caps.maxImageCount < caps.minImageCount) { return std::max(3u, caps.minImageCount); }
	return std::clamp(3u, caps.minImageCount, caps.maxImageCount);
}

constexpr auto image_extent(vk::SurfaceCapabilitiesKHR const& caps, vk::Extent2D const fb) noexcept -> vk::Extent2D {
	constexpr auto limitless_v = std::numeric_limits<std::uint32_t>::max();
	if (caps.currentExtent.width < limitless_v && caps.currentExtent.height < limitless_v) { return caps.currentExtent; }
	auto const x = std::clamp(fb.width, caps.minImageExtent.width, caps.maxImageExtent.width);
	auto const y = std::clamp(fb.height, caps.minImageExtent.height, caps.maxImageExtent.height);
	return vk::Extent2D{x, y};
}

auto optimal_depth_format(vk::PhysicalDevice const gpu) -> vk::Format {
	static constexpr auto target{vk::Format::eD32Sfloat};
	auto const props = gpu.getFormatProperties(target);
	if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) { return target; }
	return vk::Format::eD16Unorm;
}

auto const g_log{logger::Logger{"Renderer"}};
} // namespace

auto Renderer::Frame::make(vk::Device const device, std::uint32_t const queue_family, vk::Format depth_format) -> Frame {
	auto ret = Frame{};

	auto const ici = ImageCreateInfo{
		.format = depth_format,
		.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
		.aspect = vk::ImageAspectFlagBits::eDepth,
		.view_type = vk::ImageViewType::e2D,
		.mip_map = false,
	};
	fill_buffered(ret.depth_images, [ici] { return std::make_unique<Image>(ici); });

	for (auto& sync : ret.syncs) {
		sync.command_pool = device.createCommandPoolUnique(vk::CommandPoolCreateInfo{vk::CommandPoolCreateFlagBits::eTransient, queue_family});
		auto const cbai = vk::CommandBufferAllocateInfo{*sync.command_pool, vk::CommandBufferLevel::ePrimary, 1};
		if (device.allocateCommandBuffers(&cbai, &sync.command_buffer) != vk::Result::eSuccess) { throw Error{"Failed to allocate Vulkan Command Buffer"}; }
		sync.draw = device.createSemaphoreUnique({});
		sync.present = device.createSemaphoreUnique({});
		sync.drawn = device.createFenceUnique({vk::FenceCreateFlagBits::eSignaled});
	}
	return ret;
}

Renderer::Renderer(glm::uvec2 const framebuffer_extent) {
	auto& device = Device::self();

	m_swapchain.present_modes = device.get_physical_device().getSurfacePresentModesKHR(device.get_surface());
	m_swapchain.formats = Swapchain::Formats::make(device.get_physical_device().getSurfaceFormatsKHR(device.get_surface()));
	m_swapchain.create_info = m_swapchain.make_create_info(device.get_surface(), device.get_queue_family());

	recreate_swapchain(framebuffer_extent);

	m_frame = Frame::make(device.get_device(), device.get_queue_family(), optimal_depth_format(device.get_physical_device()));

	auto const line_width_range = device.get_physical_device().getProperties().limits.lineWidthRange;
	m_line_width_limit = {line_width_range[0], line_width_range[1]};
}

Renderer::~Renderer() {
	Device::self().get_device().waitIdle();
	m_defer.clear();
}

auto Renderer::get_colour_format() const -> vk::Format { return m_swapchain.create_info.imageFormat; }

auto Renderer::get_depth_format() const -> vk::Format { return m_frame.depth_images[0]->format(); }

auto Renderer::wait_for_frame(glm::uvec2 const framebuffer_extent) -> std::optional<std::uint32_t> {
	if (framebuffer_extent.x == 0 || framebuffer_extent.y == 0) { return {}; }

	auto ret = acquire_next_image(framebuffer_extent);
	if (!ret) {
		m_imgui->new_frame();
		return {};
	}

	auto& device = Device::self();
	auto& sync = m_frame.syncs[get_frame_index()];

	if (!device.reset(*sync.drawn)) { throw Error{"Failed to wait for frame fence"}; }

	device.get_device().resetCommandPool(*sync.command_pool);
	m_defer.next_frame();
	if (!m_swapchain.retired.empty()) { m_swapchain.retired.pop_front(); }
	m_imgui->new_frame();
	m_descriptor_cache.next_frame();
	m_scratch_buffer_cache.next_frame();

	m_frame.framebuffer_extent = framebuffer_extent;
	m_frame.last_bound = vk::Pipeline{};

	return ret;
}

auto Renderer::render(std::span<NotNull<Subpass*> const> passes, std::uint32_t const image_index) -> std::uint32_t {
	auto& sync = m_frame.syncs[get_frame_index()];
	sync.command_buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	auto draw_calls = std::uint32_t{};

	auto const swapchain_image = m_swapchain.active.images[image_index];
	auto& depth_image = m_frame.depth_images[get_frame_index()];
	if (depth_image->extent() != swapchain_image.extent) { depth_image->recreate(swapchain_image.extent); }

	auto colour_image_barrier = ImageBarrier{swapchain_image.image};
	colour_image_barrier.set_undef_to_optimal(false).transition(sync.command_buffer);
	auto depth_image_barrier = ImageBarrier{*depth_image};
	depth_image_barrier.set_undef_to_optimal(true).transition(sync.command_buffer);

	auto const depth_image_view = ImageView{
		.image = depth_image->image(),
		.view = depth_image->image_view(),
		.extent = depth_image->extent(),
		.format = depth_image->format(),
	};

	auto execute_pass = [&](Subpass& pass) {
		m_rendering = true;
		m_current_pass = &pass;
		pass.do_setup({.colour = swapchain_image, .depth = depth_image_view}, m_line_width_limit);
		pass.do_render(sync.command_buffer);
		draw_calls += pass.m_data.draw_calls;
		m_rendering = false;
	};

	for (auto const& pass : passes) { execute_pass(*pass); }
	execute_pass(*m_imgui);

	colour_image_barrier.set_optimal_to_present();
	colour_image_barrier.transition(sync.command_buffer);

	sync.command_buffer.end();

	return draw_calls;
}

auto Renderer::submit_frame(std::uint32_t const image_index) -> bool {
	auto& sync = m_frame.syncs[get_frame_index()];

	auto vsi = vk::SubmitInfo2{};
	auto const cbsi = vk::CommandBufferSubmitInfo{sync.command_buffer};
	auto const ssi_wait = vk::SemaphoreSubmitInfo{*sync.draw, {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput};
	auto const ssi_signal = vk::SemaphoreSubmitInfo{*sync.present, {}, vk::PipelineStageFlagBits2::eColorAttachmentOutput};
	vsi.commandBufferInfoCount = 1;
	vsi.pCommandBufferInfos = &cbsi;
	vsi.waitSemaphoreInfoCount = 1;
	vsi.pWaitSemaphoreInfos = &ssi_wait;
	vsi.signalSemaphoreInfoCount = 1;
	vsi.pSignalSemaphoreInfos = &ssi_signal;

	auto vpi = vk::PresentInfoKHR{};
	vpi.pWaitSemaphores = &*sync.present;
	vpi.waitSemaphoreCount = 1u;
	vpi.pImageIndices = &image_index;
	vpi.pSwapchains = &*m_swapchain.active.swapchain;
	vpi.swapchainCount = 1u;

	auto const result = Device::self().submit_and_present(vsi, *sync.drawn, vpi);
	m_frame.frame_index.increment();

	switch (result) {
	case vk::Result::eSuccess: break;
	case vk::Result::eSuboptimalKHR:
	case vk::Result::eErrorOutOfDateKHR: recreate_swapchain(m_frame.framebuffer_extent); return false;
	default: throw Error{"Failed to present Swapchain Image"};
	}

	return true;
}

auto Renderer::recreate_swapchain(std::optional<glm::uvec2> extent, std::optional<vk::PresentModeKHR> mode) -> bool {
	auto& device = Device::self();

	auto const caps = device.get_physical_device().getSurfaceCapabilitiesKHR(device.get_surface());
	if (extent) {
		if (extent->x == 0 || extent->y == 0) { return false; }
		m_swapchain.create_info.imageExtent = image_extent(caps, vk::Extent2D{extent->x, extent->y});
		m_swapchain.active.extent = {m_swapchain.create_info.imageExtent.width, m_swapchain.create_info.imageExtent.height};
	}
	if (mode) {
		assert(std::ranges::find(m_swapchain.present_modes, *mode) != m_swapchain.present_modes.end());
		m_swapchain.create_info.presentMode = *mode;
	}
	assert(m_swapchain.create_info.imageExtent.width > 0 && m_swapchain.create_info.imageExtent.height > 0);
	auto info = m_swapchain.create_info;
	info.minImageCount = image_count(caps);
	info.oldSwapchain = m_swapchain.active.swapchain.get();
	auto new_swapchain = device.get_device().createSwapchainKHRUnique(info);

	m_swapchain.create_info = info;
	if (m_swapchain.active.swapchain) { m_swapchain.retired.push_back(std::move(m_swapchain.active)); }
	m_swapchain.active.swapchain = std::move(new_swapchain);
	auto count = std::uint32_t{};
	if (device.get_device().getSwapchainImagesKHR(*m_swapchain.active.swapchain, &count, nullptr) != vk::Result::eSuccess) {
		throw Error{"Failed to get Swapchain Images"};
	}

	m_swapchain.active.images.resize(count);
	auto const images = device.get_device().getSwapchainImagesKHR(*m_swapchain.active.swapchain);
	m_swapchain.active.images.clear();
	m_swapchain.active.views.clear();
	for (auto const image : images) {
		auto const isr = vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
		auto ivci = vk::ImageViewCreateInfo{};
		ivci.viewType = vk::ImageViewType::e2D;
		ivci.format = m_swapchain.create_info.imageFormat;
		ivci.components.r = ivci.components.g = ivci.components.b = ivci.components.a = vk::ComponentSwizzle::eIdentity;
		ivci.subresourceRange = isr;
		ivci.image = image;
		m_swapchain.active.views.push_back(device.get_device().createImageViewUnique(ivci));
		m_swapchain.active.images.push_back({
			.image = image,
			.view = *m_swapchain.active.views.back(),
			.extent = m_swapchain.create_info.imageExtent,
			.format = m_swapchain.create_info.imageFormat,
		});
	}

	g_log.info("Swapchain extent: [{}x{}] | images: [{}] | colour space: [{}] | vsync: [{}]", m_swapchain.create_info.imageExtent.width,
			   m_swapchain.create_info.imageExtent.height, m_swapchain.active.images.size(), Swapchain::is_srgb_format(info.imageFormat) ? "sRGB" : "linear",
			   to_vsync_string(info.presentMode));

	return true;
}

auto Renderer::bind_pipeline(vk::Pipeline pipeline) -> bool {
	if (!m_rendering || !pipeline) { return false; }
	if (m_frame.last_bound != pipeline) {
		auto const cmd = m_frame.syncs[get_frame_index()].command_buffer;
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		m_frame.last_bound = pipeline;
	}
	assert(m_current_pass);
	set_viewport();
	return true;
}

auto Renderer::set_viewport(vk::Viewport viewport) -> bool {
	if (!m_rendering || !m_frame.last_bound) { return false; }
	if (viewport == vk::Viewport{}) { viewport = to_viewport(m_frame.framebuffer_extent); }
	auto const flipped_viewport = vk::Viewport{viewport.x, viewport.height + viewport.y, viewport.width, -viewport.height, 0.0f, 1.0f};
	auto command_buffer = m_frame.syncs[get_frame_index()].command_buffer;
	command_buffer.setViewport(0, flipped_viewport);
	command_buffer.setScissor(0, to_rect2d(viewport));
	return true;
}

auto Renderer::set_scissor(vk::Rect2D scissor) -> bool {
	if (!m_rendering || !m_frame.last_bound) { return false; }
	m_frame.syncs[get_frame_index()].command_buffer.setScissor(0, scissor);
	return true;
}

auto Renderer::acquire_next_image(glm::uvec2 const framebuffer_extent) -> std::optional<std::uint32_t> {
	auto& device = Device::self();
	auto& sync = m_frame.syncs[get_frame_index()];

	auto image_index = std::uint32_t{};
	auto const result = device.acquire_next_image(*m_swapchain.active.swapchain, image_index, *sync.draw);

	switch (result) {
	case vk::Result::eSuccess: return image_index;
	case vk::Result::eSuboptimalKHR:
	case vk::Result::eErrorOutOfDateKHR: recreate_swapchain(framebuffer_extent); return {};
	default: throw Error{"Failed to acquire Swapchain Image"};
	}
}
} // namespace le::graphics
