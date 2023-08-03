#include <spaced/graphics/cache/pipeline_cache.hpp>
#include <spaced/graphics/descriptor_updater.hpp>
#include <spaced/graphics/renderer.hpp>
#include <spaced/graphics/subpass.hpp>

namespace spaced::graphics {
namespace {
struct RenderingInfoBuilder {
	vk::RenderingAttachmentInfo colour{};
	vk::RenderingAttachmentInfo depth{};

	[[nodiscard]] auto build(RenderTarget const& target, Subpass::Load const& load) -> vk::RenderingInfo {
		auto vri = vk::RenderingInfo{};

		colour.clearValue = load.clear_colour;
		colour.loadOp = load.load_op;
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
	bool const is_ortho = std::holds_alternative<Camera::Orthographic>(camera->type);
	auto const view = Std140View{
		.view = camera->view(),
		.projection = camera->projection(projection),
		.vpos_exposure = {camera->transform.position(), camera->exposure},
		.vdir_ortho = {front_v * camera->transform.orientation(), is_ortho ? 1.0f : 0.0f},
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

auto Subpass::render_objects(RenderCamera const& camera, std::span<RenderObject const> objects, vk::CommandBuffer cmd) -> void {
	static auto const default_instance{graphics::RenderInstance{}};

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
		auto const instances = object.instances.empty() ? std::span{&default_instance, 1} : object.instances;

		auto const& material = Material::or_default(object.material);
		auto const pipeline = PipelineCache::self().load(pipeline_format, &material.get_shader(), &object.pipeline_state);
		if (!Renderer::self().bind_pipeline(pipeline)) { continue; }

		cmd.setLineWidth(m_data.line_width_limit.clamp(object.pipeline_state.line_width));

		if (m_data.last_bound != &material) {
			material.bind_set(cmd);
			m_data.last_bound = &material;
		}

		auto const render_instances = build_instances(object.parent, instances);
		auto object_set = DescriptorUpdater{object_layout.set};
		object_set.write_storage(object_layout.instances, render_instances.data(), render_instances.size_bytes());
		if (!object.joints.empty()) { object_set.write_storage(object_layout.joints, object.joints.data(), std::span{object.joints}.size_bytes()); }
		object_set.bind_set(cmd);

		object.primitive->draw(static_cast<std::uint32_t>(render_instances.size()), cmd);
	}
}

auto Subpass::do_setup(RenderTarget const& swapchain, InclusiveRange<float> line_width_limit) -> void {
	m_data = Data{.render_target = swapchain, .swapchain_image = swapchain.colour, .line_width_limit = line_width_limit};
	m_data.projection = glm::uvec2{m_data.swapchain_image.extent.width, m_data.swapchain_image.extent.height};
}

void Subpass::do_render(vk::CommandBuffer cmd) {
	auto builder = RenderingInfoBuilder{};
	auto const load = get_load();
	auto const vri = builder.build(m_data.render_target, load);
	cmd.beginRendering(vri);
	render(cmd);
	cmd.endRendering();
}
} // namespace spaced::graphics
