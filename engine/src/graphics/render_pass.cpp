#include <spaced/graphics/cache/pipeline_cache.hpp>
#include <spaced/graphics/descriptor_updater.hpp>
#include <spaced/graphics/render_pass.hpp>
#include <spaced/graphics/renderer.hpp>

namespace spaced::graphics {
namespace {
struct RenderingInfoBuilder {
	vk::RenderingAttachmentInfo colour{};
	vk::RenderingAttachmentInfo depth{};

	[[nodiscard]] auto build(RenderTarget const& target, vk::ClearColorValue const& clear_colour, vk::AttachmentLoadOp load_op) -> vk::RenderingInfo {
		auto vri = vk::RenderingInfo{};

		colour.clearValue = clear_colour;
		colour.loadOp = load_op;
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
};
} // namespace

auto RenderCamera::bind_set(glm::vec2 const projection, vk::CommandBuffer const cmd) const -> void {
	auto const view = Std140View{
		.mat_vp = camera->projection(projection) * camera->view(),
		.vpos_exposure = {camera->transform.position(), camera->exposure},
	};

	auto dir_lights = std::vector<Std430DirLight>{};
	dir_lights.reserve(lights->directional.size());
	for (auto const& in : lights->directional) {
		dir_lights.push_back(Std430DirLight{
			.direction = glm::vec4{in.direction.value(), 0.0f},
			.diffuse = in.diffuse.to_vec4(),
			.ambient = in.diffuse.Rgba::to_vec4(in.ambient),
		});
	}

	auto const& layout = PipelineCache::self().shader_layout().camera;
	DescriptorUpdater{layout.set}
		.write_storage(layout.directional_lights, dir_lights.data(), std::span{dir_lights}.size_bytes())
		.write_uniform(layout.view, &view, sizeof(view))
		.bind_set(cmd);
}

auto RenderPass::render_objects(RenderCamera const& camera, std::span<RenderObject const> objects, vk::CommandBuffer cmd) -> void {
	auto const pipeline_format = PipelineFormat{.colour = m_data.render_target.colour.format, .depth = m_data.render_target.depth.format};
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

	camera.bind_set(m_data.projection, cmd);

	for (auto const& object : objects) {
		if (object.instances.empty()) { continue; }

		auto const& material = Material::or_default(object.material);
		auto const& shader = material.get_shader();
		auto const pipeline = PipelineCache::self().load(pipeline_format, shader.vertex, shader.fragment, object.pipeline_state);
		if (!Renderer::self().bind_pipeline(pipeline)) { continue; }

		if (m_data.last_bound != &material) {
			material.bind_set(cmd);
			m_data.last_bound = &material;
		}

		auto const instances = build_instances(object.parent, object.instances);
		auto object_set = DescriptorUpdater{object_layout.set};
		object_set.write_storage(object_layout.instances, instances.data(), instances.size_bytes());
		if (!object.joints.empty()) { object_set.write_storage(object_layout.joints, object.joints.data(), std::span{object.joints}.size_bytes()); }
		object_set.bind_set(cmd);

		object.primitive->draw(static_cast<std::uint32_t>(object.instances.size()), cmd);
	}
}

auto RenderPass::do_setup(RenderTarget const& swapchain) -> void {
	m_data = Data{.render_target = swapchain, .swapchain_image = swapchain.colour};
	m_data.projection = glm::uvec2{m_data.swapchain_image.extent.width, m_data.swapchain_image.extent.height};
	setup_framebuffer();
}

void RenderPass::do_render(vk::CommandBuffer cmd) {
	auto builder = RenderingInfoBuilder{};
	auto const vri = builder.build(m_data.render_target, clear_colour, m_colour_load_op);
	cmd.beginRendering(vri);
	render(cmd);
	cmd.endRendering();
}
} // namespace spaced::graphics
