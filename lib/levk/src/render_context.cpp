#include <levk/assets/shader_asset.hpp>
#include <levk/core/enum_array.hpp>
#include <levk/core/error.hpp>
#include <levk/render_context.hpp>
#include <levk/render_device.hpp>
#include <levk/util.hpp>
#include <unordered_set>

namespace levk {
namespace {
constexpr auto to_set_number(DescriptorSetType const set_type) { return static_cast<std::uint32_t>(set_type); }

constexpr auto clamp_line_width(float const desired, std::span<float const, 2> range) { return std::clamp(desired, range[0], range[1]); }

constexpr auto shadow_sampler_v = TextureSampler{
	.wrap_u = vk::SamplerAddressMode::eClampToBorder,
	.wrap_v = vk::SamplerAddressMode::eClampToBorder,
	.wrap_w = vk::SamplerAddressMode::eClampToBorder,
	.min_filter = vk::Filter::eNearest,
	.mag_filter = vk::Filter::eNearest,
	.border = vk::BorderColor::eIntOpaqueWhite,
	.anisotropy = 0.0f,
};

struct DescriptorWrite {
	vk::DescriptorSet descriptor_set{};
	FlexArray<DescriptorInfo, IMaterial::DescriptorBuffer::capacity_v> infos{};
};

struct DescriptorBinder {
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
	IPipeline& pipeline;

	EnumArray<DescriptorSetType, vk::DescriptorSet> descriptor_sets{};
	struct {
		FlexArray<DescriptorInfo, 4> view{};
		DescriptorInfo instances{};
		IMaterial::DescriptorBuffer material{};
		DescriptorInfo joints{};
	} descriptors{};

	auto allocate_descriptor_set(DescriptorSetType const set_type) -> vk::DescriptorSet {
		auto& set = descriptor_sets[set_type];
		if (set) { return set; }
		auto const set_number = to_set_number(set_type);
		if (pipeline.get_bindings(set_number).empty()) { return {}; }
		set = pipeline.allocate_descriptor_set(set_number);
		return set;
	}

	void write_view_lights_shadow(IUniformBuffer const& view, IUniformBuffer const& lights, ITexture const& shadow_map) {
		descriptors.view.clear();
		descriptors.view.insert(view.get_descriptor_info(0));
		if (pipeline.is_writable(to_set_number(DescriptorSetType::eView), 1)) { descriptors.view.insert(lights.get_descriptor_info(1)); }
		if (pipeline.is_writable(to_set_number(DescriptorSetType::eView), 2)) { descriptors.view.insert(shadow_map.get_descriptor_info(2)); }
	}

	void write_instances(IStorageBuffer const& instances) { descriptors.instances = instances.get_descriptor_info(0); }

	void write_material(IMaterial const& material) {
		descriptors.material.clear();
		material.push_descriptors(descriptors.material);
	}

	void write_joints(Ptr<IStorageBuffer const> joints) {
		if (joints == nullptr) { return; }
		descriptors.joints = joints->get_descriptor_info(0);
	}

	void bind_descriptor_sets(vk::CommandBuffer const command_buffer) {
		static constexpr std::size_t size_v{IMaterial::DescriptorBuffer::capacity_v + 4};
		auto writes = FlexArray<vk::WriteDescriptorSet, size_v>{};
		auto insert_write = [&](DescriptorInfo const& descriptor, vk::DescriptorSet const set) {
			if (!set) { return; }
			auto write = descriptor.get_write_descriptor_set(set);
			if (write.pBufferInfo == nullptr && write.pImageInfo == nullptr) { return; }
			writes.insert(write);
		};

		if (!descriptors.view.empty()) {
			auto view_set = allocate_descriptor_set(DescriptorSetType::eView);
			for (auto const& descriptor : descriptors.view.span()) { insert_write(descriptor, view_set); }
		}

		if (descriptors.instances.has_value()) {
			auto instances_set = allocate_descriptor_set(DescriptorSetType::eInstances);
			insert_write(descriptors.instances, instances_set);
		}

		if (!descriptors.material.empty()) {
			auto material_set = allocate_descriptor_set(DescriptorSetType::eMaterial);
			for (auto const& descriptor : descriptors.material.span()) { insert_write(descriptor, material_set); }
		}

		if (descriptors.joints.has_value()) {
			auto joints_set = allocate_descriptor_set(DescriptorSetType::eJoints);
			insert_write(descriptors.joints, joints_set);
		}

		if (!writes.empty()) { pipeline.update_descriptor_sets(writes.span()); }
		for (auto const [index, descriptor_set] : std::ranges::enumerate_view(descriptor_sets)) {
			if (!descriptor_set) { continue; }
			pipeline.bind_descriptor_set(command_buffer, descriptor_set, static_cast<std::uint32_t>(index));
		}

		descriptors = {};
		descriptor_sets = {};
	}
};
} // namespace

auto RenderContext::create(NotNull<IRenderDevice*> render_device) -> std::optional<RenderContext> {
	auto const command_buffer = render_device->acquire_next_image();
	if (!command_buffer) { return {}; }
	return RenderContext{render_device, command_buffer};
}

RenderContext::RenderContext(NotNull<IRenderDevice*> render_device, vk::CommandBuffer const command_buffer)
	: m_device(render_device), m_command_buffer(command_buffer) {}

auto RenderContext::set_shadow_fragment_shader(IAssetStore& asset_store, std::string_view const shader_uri) -> bool {
	return set_shader(m_shadow_fs, asset_store, shader_uri);
}

auto RenderContext::set_skybox_vertex_shader(IAssetStore& asset_store, std::string_view const shader_uri) -> bool {
	return set_shader(m_skybox.vertex_shader, asset_store, shader_uri);
}

auto RenderContext::set_skybox_fragment_shader(IAssetStore& asset_store, std::string_view const shader_uri) -> bool {
	return set_shader(m_skybox.fragment_shader, asset_store, shader_uri);
}

void RenderContext::add_objects(std::span<RenderObject const> objects) {
	for (auto const& object : objects) {
		auto baked = BakedObject{};
		if (!bake(object, baked)) { continue; }
		m_objects.push_back(baked);
	}
}

void RenderContext::add_skybox(NotNull<ICubemap const*> cubemap) {
	if (m_skybox.vertex_shader.empty()) {
		m_log.error("RenderContext::add_skybox(): skybox vertex shader not set");
		return;
	}
	if (m_skybox.fragment_shader.empty()) {
		m_log.error("RenderContext::add_skybox(): skybox fragment shader not set");
		return;
	}

	if (!m_skybox.material) { m_skybox.material = std::make_unique<material::Unlit>(*m_device, m_skybox.fragment_shader); }
	if (!m_skybox.cube) {
		auto const geometry = Geometry::from(shape::Cube{.size = glm::vec3{1.0f}});
		m_skybox.cube = m_device->create_static_primitive(geometry, m_skybox.material.get(), m_skybox.vertex_shader);
		m_skybox.primitive.emplace(m_skybox.cube.get());
	}

	m_skybox.material->texture = cubemap;

	auto const object = RenderObject{
		.primitives = {&*m_skybox.primitive, 1},
		.instances = {&m_skybox.instance, 1},
		.depth_compare = vk::CompareOp::eLessOrEqual,
		.alpha_blend = false,
	};
	auto baked = BakedObject{};
	if (!bake(object, baked)) { return; }

	m_skybox.baked = baked;
}

auto RenderContext::set_camera(NotNull<IRenderCamera*> camera) -> RenderContext& {
	m_camera = camera;
	return *this;
}

auto RenderContext::set_view(RenderView const& view) -> RenderContext& {
	m_view = view;
	return *this;
}

auto RenderContext::draw_shadows(glm::ivec2 const resolution, glm::vec3 const& projection_viewport) -> RenderStats {
	if (m_shadow_fs.empty()) {
		m_log.error("RenderContext::draw_shadows(): shadow fragment shader not set");
		return {};
	}
	if (!is_positive(resolution)) {
		m_log.error("RenderContext::draw_shadows(): invalid resolution: {}x{}", resolution.x, resolution.y);
		return {};
	}

	auto const camera_position = glm::vec3{m_view.camera_transform[3]};
	auto const target = m_view.main_light.orientation * front_v;
	m_shadow_view = glm::lookAt(camera_position - target, camera_position, up_v);
	auto const half_size = 0.5f * projection_viewport;
	m_shadow_proj = glm::ortho(-half_size.x, half_size.x, -half_size.y, half_size.y, -half_size.z, half_size.z);

	auto const rbi = RenderBeginInfo{
		.camera_view = m_shadow_view,
		.camera_proj = m_shadow_proj,
	};
	auto const ret = draw(resolution, rbi, DrawType::eShadows);

	m_shadow_map = &m_camera->get_render_texture(shadow_sampler_v);

	return ret;
}

auto RenderContext::draw_renderers(glm::ivec2 const resolution, Degrees const field_of_view, glm::vec2 const z_plane) -> RenderStats {
	if (m_camera == nullptr) {
		m_log.error("RenderContext::draw_renderers(): RenderCamera not set");
		return {};
	}
	if (!is_positive(resolution)) {
		m_log.error("RenderContext::draw_renderers(): invalid resolution: {}x{}", resolution.x, resolution.y);
		return {};
	}

	auto const fresolution = glm::vec2{resolution};
	auto const aspect_ratio = fresolution.x / fresolution.y;
	auto camera_transform = Transform{};
	camera_transform.from_matrix(m_view.camera_transform);
	auto const rotation_matrix = glm::toMat4(glm::inverse(camera_transform.get_orientation()));
	auto const view_matrix = rotation_matrix * glm::translate(identity_mat_v, -camera_transform.get_position());
	auto const projection_matrix = glm::perspective(Radians{field_of_view}.value, aspect_ratio, z_plane.x, z_plane.y);

	auto const rbi = RenderBeginInfo{
		.main_light = m_view.main_light,
		.camera_position = camera_transform.get_position(),
		.camera_exposure = m_view.camera_exposure,
		.camera_view = view_matrix,
		.camera_proj = projection_matrix,
		.shadow_view_proj = m_shadow_proj * m_shadow_view,
	};
	return draw(resolution, rbi, DrawType::eRenderers);
}

void RenderContext::submit() {
	m_device->submit_and_present(m_last_rt);
	clear();
}

void RenderContext::clear() {
	m_objects.clear();
	m_instances.clear();
	m_shadow_map = {};
	m_shadow_fs = {};
	m_last_rt = {};
	m_skybox.baked = {};
}

auto RenderContext::set_shader(RenderShader& out, IAssetStore& asset_store, std::string_view const shader_uri) -> bool {
	auto const* asset = asset_store.load<ShaderAsset>(shader_uri);
	if (asset == nullptr) {
		m_log.error("Failed to load shader: '{}'", shader_uri);
		return false;
	}
	if (asset->get_render_shader().empty()) {
		m_log.error("Invalid shader: '{}'", shader_uri);
		return false;
	}

	out = asset->get_render_shader();
	return true;
}

auto RenderContext::bake(RenderObject const& object, BakedObject& out) -> bool {
	if (object.instances.empty()) { return false; }

	out.instance_count = static_cast<std::uint32_t>(object.instances.size());
	out.primitives = object.primitives;
	out.line_width = clamp_line_width(object.line_width, m_device->get_properties().limits.lineWidthRange);
	out.polygon_mode = object.polygon_mode;
	out.disable_depth_test = object.disable_depth_test;
	out.depth_compare = object.depth_compare;
	out.alpha_blend = object.alpha_blend;

	m_instances.clear();
	m_instances.reserve(object.instances.size());
	for (auto const& instance : object.instances) { m_instances.push_back(instance.bake(object.parent)); }
	auto& instance_buffer = m_device->allocate_storage_buffer();
	instance_buffer.set_data(m_instances.data(), std::span{m_instances}.size_bytes());
	out.instances = &instance_buffer;

	if (!object.joint_matrices.empty()) {
		auto& joint_mats_buffer = m_device->allocate_storage_buffer();
		joint_mats_buffer.set_data(object.joint_matrices.data(), object.joint_matrices.size_bytes());
		out.joint_matrices = &joint_mats_buffer;
	}

	return true;
}

auto RenderContext::get_pipeline(BakedObject const& object, IPrimitive const& primitive, DrawType type) const -> Ptr<IPipeline> {
	assert(m_camera);

	auto fragment_shader = primitive.material->fragment_shader;
	auto vertex_binding = primitive.get_vertex_binding();
	if (type == DrawType::eShadows) {
		if (m_shadow_fs.empty()) { return {}; }
		fragment_shader = m_shadow_fs;
	}

	auto const primitive_state = RenderPrimitiveState{
		.topology = primitive.get_topology(),
		.vertex_binding = vertex_binding,
		.alpha_blend = object.alpha_blend,
	};
	auto pass_state = m_camera->get_render_state();
	if (object.disable_depth_test) { pass_state.depth_test = false; }
	if (object.polygon_mode) { pass_state.polygon_mode = *object.polygon_mode; }
	if (object.depth_compare) { pass_state.depth_compare = *object.depth_compare; }

	auto const pipeline_state = PipelineState{
		.primitive_state = primitive_state,
		.pass_state = pass_state,
	};

	return m_device->get_pipeline(primitive.vertex_shader, fragment_shader, pipeline_state);
}

auto RenderContext::bind(IPipeline& pipeline) -> bool {
	assert(m_camera);

	if (&pipeline == m_previous.pipeline) { return false; }

	m_previous = {.pipeline = &pipeline};

	m_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get_pipeline());
	auto const render_target = m_camera->get_render_target();
	glm::vec2 const viewport_size = to_glm_vec2(render_target.extent);
	m_command_buffer.setScissor(0, vk::Rect2D{vk::Offset2D{}, render_target.extent});
	m_command_buffer.setViewport(0, vk::Viewport{0.0f, viewport_size.y, viewport_size.x, -viewport_size.y, 0.0f, 1.0f});

	auto binder = DescriptorBinder{pipeline};

	auto const* shadow_texture = m_shadow_map != nullptr ? m_shadow_map : &m_device->get_fallback_texture();
	binder.write_view_lights_shadow(m_camera->get_view_buffer(), m_camera->get_main_light_buffer(), *shadow_texture);
	binder.bind_descriptor_sets(m_command_buffer);

	return true;
}

auto RenderContext::draw(glm::ivec2 resolution, RenderBeginInfo const& info, DrawType type) -> RenderStats {
	assert(m_camera);

	auto ret = RenderStats{};
	auto pipelines = std::unordered_set<Ptr<IPipeline>>{};

	auto const render_start = Clock::now();
	m_camera->begin_rendering(info, resolution, m_command_buffer);

	auto const draw_primitive = [&](BakedObject const& object, IPrimitive const& primitive) {
		auto* pipeline = get_pipeline(object, primitive, type);
		if (pipeline == nullptr) { return; }

		pipelines.insert(pipeline);
		if (bind(*pipeline)) { ++ret.pipeline_binds; }
		draw(*pipeline, object, primitive, type);

		++ret.draw_calls;
		ret.triangles += primitive.get_triangle_count();
	};

	for (auto const& object : m_objects) {
		for (auto const primitive : object.primitives) { draw_primitive(object, *primitive); }
	}

	if (type == DrawType::eRenderers && m_skybox.baked) { draw_primitive(*m_skybox.baked, **m_skybox.primitive); }

	m_last_rt = m_camera->end_rendering();
	ret.render_time = Clock::now() - render_start;

	m_previous = {};
	ret.pipelines = static_cast<std::int64_t>(pipelines.size());

	return ret;
}

void RenderContext::draw(IPipeline& pipeline, BakedObject const& object, IPrimitive const& primitive, DrawType const type) {
	auto binder = DescriptorBinder{pipeline};

	if (type == DrawType::eRenderers) {
		if (m_previous.material != primitive.material) {
			m_previous.material = primitive.material;
			binder.write_material(*primitive.material);
		}
	}

	if (m_previous.instances != object.instances) {
		m_previous.instances = object.instances;
		binder.write_instances(*object.instances);
	}

	if (m_previous.joints != object.joint_matrices) {
		m_previous.joints = object.joint_matrices;
		binder.write_joints(object.joint_matrices);
	}

	binder.bind_descriptor_sets(m_command_buffer);

	m_command_buffer.setLineWidth(object.line_width);

	auto const vbo_view = primitive.get_vertex_array();
	auto const skin_view = primitive.get_vertex_skin();
	m_command_buffer.bindVertexBuffers(0, vbo_view.buffer, vk::DeviceSize{});
	if (skin_view.buffer) {
		auto const buffers = std::array{skin_view.buffer, skin_view.buffer};
		auto const offsets = std::array{vk::DeviceSize{}, skin_view.weights_offset};
		// TODO: use constant here and in vertex layout.
		m_command_buffer.bindVertexBuffers(5, buffers, offsets);
	}
	if (vbo_view.index_count > 0) {
		m_command_buffer.bindIndexBuffer(vbo_view.buffer, vbo_view.index_offset, vk::IndexType::eUint32);
		m_command_buffer.drawIndexed(vbo_view.index_count, object.instance_count, 0, 0, 0);
	} else {
		m_command_buffer.draw(vbo_view.vertex_count, object.instance_count, 0, 0);
	}
}
} // namespace levk
