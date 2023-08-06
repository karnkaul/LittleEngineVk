#include <le/core/logger.hpp>
#include <le/graphics/descriptor_updater.hpp>
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

struct RenderingInfoBuilder {
	vk::RenderingAttachmentInfo colour{};
	vk::RenderingAttachmentInfo depth{};

	[[nodiscard]] auto build(RenderTarget const& target, std::optional<vk::ClearColorValue> const& clear) -> vk::RenderingInfo {
		auto vri = vk::RenderingInfo{};

		if (clear) {
			colour.clearValue = *clear;
			colour.loadOp = vk::AttachmentLoadOp::eClear;
		} else {
			colour.loadOp = vk::AttachmentLoadOp::eLoad;
		}
		colour.storeOp = vk::AttachmentStoreOp::eStore;
		colour.imageView = target.colour.view;
		colour.imageLayout = vk::ImageLayout::eAttachmentOptimal;

		vri.renderArea = vk::Rect2D{{}, target.colour.extent};
		vri.colorAttachmentCount = 1;
		vri.pColorAttachments = &colour;

		if (target.depth.view != vk::ImageView{}) {
			depth.clearValue = vk::ClearDepthStencilValue{1.0f, 0};
			depth.loadOp = vk::AttachmentLoadOp::eClear;
			depth.storeOp = vk::AttachmentStoreOp::eDontCare;
			depth.imageView = target.depth.view;
			depth.imageLayout = vk::ImageLayout::eAttachmentOptimal;

			vri.pDepthAttachment = &depth;
		}

		vri.layerCount = 1;

		return vri;
	}

	[[nodiscard]] auto build_shadow(ImageView const& shadow_map) -> vk::RenderingInfo {
		auto vri = vk::RenderingInfo{};

		vri.renderArea = vk::Rect2D{{}, shadow_map.extent};

		depth.clearValue = vk::ClearDepthStencilValue{1.0f, 0};
		depth.loadOp = vk::AttachmentLoadOp::eClear;
		depth.storeOp = vk::AttachmentStoreOp::eStore;
		depth.imageView = shadow_map.view;
		depth.imageLayout = vk::ImageLayout::eAttachmentOptimal;

		vri.pDepthAttachment = &depth;

		vri.layerCount = 1;

		return vri;
	}
};

struct RenderCamera {
	struct Std140View {
		glm::mat4 view;
		glm::mat4 projection;
		glm::vec4 vpos_exposure;
		glm::vec4 vdir_ortho;
	};

	struct Std430DirLight {
		glm::vec4 direction;
		glm::vec4 diffuse;
		glm::vec4 ambient;
	};

	auto bind_set(glm::vec2 projection, ImageView const& shadow_map, vk::CommandBuffer cmd) const -> void {
		bool const is_ortho = std::holds_alternative<Camera::Orthographic>(camera->type);
		auto const view = Std140View{
			.view = camera->view(),
			.projection = camera->projection(projection),
			.vpos_exposure = {camera->transform.position(), camera->exposure},
			.vdir_ortho = {front_v * camera->transform.orientation(), is_ortho ? 1.0f : 0.0f},
		};

		auto dir_lights = std::vector<Std430DirLight>{};
		dir_lights.reserve(lights->directional.size() + 1);
		static constexpr auto to_std430_light = [](Lights::Directional const& in) {
			return Std430DirLight{
				.direction = glm::vec4{in.direction.value(), 0.0f},
				.diffuse = in.diffuse.to_vec4(),
				.ambient = in.diffuse.Rgba::to_vec4(in.ambient),
			};
		};
		dir_lights.push_back(to_std430_light(lights->primary));
		for (auto const& in : lights->directional) { dir_lights.push_back(to_std430_light(in)); }

		auto const& layout = PipelineCache::self().shader_layout().camera;
		auto set = DescriptorUpdater{layout.set};
		set.write_storage(layout.directional_lights, dir_lights.data(), std::span{dir_lights}.size_bytes()).write_uniform(layout.view, &view, sizeof(view));

		if (shadow_map.view) {
			auto const& sampler = SamplerCache::self().get({});
			auto dii = vk::DescriptorImageInfo{sampler, shadow_map.view, vk::ImageLayout::eAttachmentOptimal};
			set.update(layout.shadow_map, dii, 1);
		}
		set.bind_set(cmd);
	}

	NotNull<Camera const*> camera;
	NotNull<Lights const*> lights;
};

auto const g_log{logger::Logger{"Renderer"}};
} // namespace

struct Renderer::Pass {
	Renderer& renderer; // NOLINT
	RenderTarget render_target{};
	ImageView shadow_map{};
	glm::vec2 projection{};

	mutable Ptr<Material const> last_bound{};

	auto render_list(RenderCamera const& camera, BakedList const list, vk::CommandBuffer cmd, bool transparent) const -> std::uint32_t {
		if (list.empty()) { return 0; }

		auto const pipeline_format = PipelineFormat{.colour = render_target.colour.format, .depth = render_target.depth.format};
		auto const& object_layout = PipelineCache::self().shader_layout().object;

		camera.bind_set(projection, shadow_map, cmd);
		auto ret = std::uint32_t{};

		for (auto const& baked : list) {
			auto const& material = Material::or_default(baked.object.material);
			if (!transparent && material.is_transparent()) { continue; }
			auto const pipeline = PipelineCache::self().load(pipeline_format, &material.get_shader(), &baked.object.pipeline_state);
			if (!renderer.bind_pipeline(pipeline)) { continue; }

			cmd.setLineWidth(renderer.m_line_width_limit.clamp(baked.object.pipeline_state.line_width));

			if (last_bound != &material) {
				material.bind_set(cmd);
				last_bound = &material;
			}

			DescriptorUpdater::bind_set(object_layout.set, baked.descriptor_set, cmd);

			baked.object.primitive->draw(baked.instance_count, cmd);
			++ret;
		}

		return ret;
	}
};

auto Renderer::Frame::make(vk::Device const device, std::uint32_t const queue_family, vk::Format depth_format) -> Frame {
	auto ret = Frame{};

	auto ici = ImageCreateInfo{
		.format = depth_format,
		.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
		.aspect = vk::ImageAspectFlagBits::eDepth,
		.view_type = vk::ImageViewType::e2D,
		.mip_map = false,
	};
	auto const make_depth_image = [&ici] { return std::make_unique<Image>(ici); };
	fill_buffered(ret.depth_images, make_depth_image);
	ici.usage |= vk::ImageUsageFlagBits::eSampled;
	fill_buffered(ret.shadow_maps, make_depth_image);

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

auto Renderer::render(RenderFrame const& render_frame, std::uint32_t const image_index) -> std::uint32_t {
	static constexpr auto ui_camera_v{Camera{.type = Camera::Orthographic{}}};

	auto& sync = m_frame.syncs[get_frame_index()];
	sync.command_buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	auto draw_calls = std::uint32_t{};

	auto const swapchain_image = m_swapchain.active.images[image_index];
	auto& depth_image = m_frame.depth_images[get_frame_index()];
	auto& shadow_map = m_frame.shadow_maps[get_frame_index()];
	if (depth_image->extent() != swapchain_image.extent) { depth_image->recreate(swapchain_image.extent); }
	if (shadow_map->extent() != shadow_map_extent) { shadow_map->recreate(shadow_map_extent); }

	bake_objects(render_frame.scene, m_scene);
	bake_objects(render_frame.ui, m_ui);

	glm::vec2 const full_projection = glm::uvec2{swapchain_image.extent.width, swapchain_image.extent.height};
	auto const shadow_map_image_view = ImageView{
		.image = shadow_map->image(),
		.view = shadow_map->image_view(),
		.extent = shadow_map->extent(),
		.format = shadow_map->format(),
	};

	auto render_target = RenderTarget{.depth = shadow_map_image_view};
	auto shadow_image_barrier = ImageBarrier{render_target.depth.image};
	shadow_image_barrier.set_undef_to_optimal(true).transition(sync.command_buffer);
	auto pass = Pass{
		.renderer = *this,
		.render_target = render_target,
		.shadow_map = {},
		.projection = glm::uvec2{shadow_frustum},
	};
	auto vri = RenderingInfoBuilder{}.build_shadow(render_target.depth);
	sync.command_buffer.beginRendering(vri);
	auto const shadow_view_plane = ViewPlane{.near = -0.5f * shadow_frustum.z, .far = 0.5f * shadow_frustum.z};
	auto shadow_camera = Camera{.type = Camera::Orthographic{.view_plane = shadow_view_plane}, .face = Camera::Face::ePositiveZ};
	shadow_camera.transform.set_orientation(Transform::look_at(render_frame.lights->primary.direction, {}));
	shadow_camera.transform.set_position(render_frame.camera->transform.position());
	auto render_camera = RenderCamera{.camera = &shadow_camera, .lights = render_frame.lights};
	m_frame.primary_light_mat = shadow_camera.projection(shadow_frustum) * shadow_camera.view();
	m_frame.backbuffer_extent = {render_target.depth.extent.width, render_target.depth.extent.height};
	m_rendering = true;
	pass.render_list(render_camera, m_scene, sync.command_buffer, false);
	m_rendering = false;
	sync.command_buffer.endRendering();
	shadow_image_barrier.set_optimal_to_read_only(true).transition(sync.command_buffer);

	m_frame.backbuffer_extent = m_frame.framebuffer_extent;
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

	render_target = RenderTarget{.colour = swapchain_image, .depth = depth_image_view};
	pass.render_target = render_target;
	pass.shadow_map = shadow_map_image_view;

	auto const clear_colour = Rgba::to_linear(render_frame.clear_colour.to_tint());
	vri = RenderingInfoBuilder{}.build(render_target, std::array{clear_colour.x, clear_colour.y, clear_colour.z, clear_colour.w});
	sync.command_buffer.beginRendering(vri);
	m_rendering = true;

	pass.projection = full_projection;
	if (render_frame.projection.x > 0.0f && render_frame.projection.y > 0.0f) { pass.projection = render_frame.projection; }
	render_camera = RenderCamera{.camera = render_frame.camera, .lights = render_frame.lights};
	pass.render_list(render_camera, m_scene, sync.command_buffer, true);

	pass.shadow_map = {};
	render_camera.camera = &ui_camera_v;
	pass.render_list(render_camera, m_ui, sync.command_buffer, true);
	m_rendering = false;
	sync.command_buffer.endRendering();

	render_target.depth = {};
	vri = RenderingInfoBuilder{}.build(render_target, {});
	sync.command_buffer.beginRendering(vri);
	m_imgui->render(sync.command_buffer);
	sync.command_buffer.endRendering();

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
	// assert(m_current_pass);
	set_viewport();
	return true;
}

auto Renderer::set_viewport(vk::Viewport viewport) -> bool {
	if (!m_rendering || !m_frame.last_bound) { return false; }
	if (viewport == vk::Viewport{}) { viewport = to_viewport(m_frame.backbuffer_extent); }
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

auto Renderer::bake_objects(std::span<RenderObject const> objects, std::vector<BakedObject>& out) -> void {
	out.clear();

	static auto const default_instance{graphics::RenderInstance{}};
	auto const& object_layout = PipelineCache::self().shader_layout().object;

	auto build_instances = [this](glm::mat4 const& parent, std::span<RenderInstance const> in) -> std::span<Std430Instance const> {
		m_instances.clear();
		m_instances.reserve(in.size());
		for (auto const& instance : in) {
			m_instances.push_back({
				.transform = parent * instance.transform.matrix(),
				.tint = Rgba::to_linear(instance.tint.to_tint()),
			});
		}
		return m_instances;
	};

	for (auto const& object : objects) {
		auto const instances = object.instances.empty() ? std::span{&default_instance, 1} : object.instances;
		auto const render_instances = build_instances(object.parent, instances);
		auto object_set = DescriptorUpdater{object_layout.set};
		object_set.write_storage(object_layout.instances, render_instances.data(), render_instances.size_bytes());
		if (!object.joints.empty()) { object_set.write_storage(object_layout.joints, object.joints.data(), std::span{object.joints}.size_bytes()); }
		out.push_back(BakedObject{
			.object = object,
			.descriptor_set = object_set.get_descriptor_set(),
			.instance_count = static_cast<std::uint32_t>(instances.size()),
		});
	}
}
} // namespace le::graphics
