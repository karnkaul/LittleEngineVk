#include <levk/materials/lit.hpp>

namespace levk::material {
Lit::Lit(IRenderDevice& render_device, RenderShader const& fragment_shader)
	: IMaterial(fragment_shader), base_colour(&render_device.get_fallback_texture()), metallic_roughness(&render_device.get_fallback_texture()),
	  emissive(&render_device.get_fallback_texture()), m_ubo(render_device.create_uniform_buffer()) {
	write_ubo();
}

void Lit::push_descriptors(DescriptorBuffer& descriptor_buffer) const {
	descriptor_buffer.insert(m_ubo->get_descriptor_info(bindings.data));

	descriptor_buffer.insert(base_colour->get_descriptor_info(bindings.base_colour));
	descriptor_buffer.insert(metallic_roughness->get_descriptor_info(bindings.metallic_roughness));
	descriptor_buffer.insert(emissive->get_descriptor_info(bindings.emissive));
	if (normal != nullptr) { descriptor_buffer.insert(normal->get_descriptor_info(bindings.normal)); }
}

void Lit::set_data(Data const& data) {
	m_data = data;
	write_ubo();
}

void Lit::write_ubo() const {
	struct Mat {
		glm::vec4 albedo;
		glm::vec4 m_r_aco_am;
		glm::vec4 emissive;
	};
	auto const is_transparent = static_cast<std::uint32_t>(m_data.is_transparent);
	auto const mat = Mat{
		.albedo = colour::to_linear(m_data.albedo),
		.m_r_aco_am = {m_data.metallic, m_data.roughness, 0.0f, std::bit_cast<float>(is_transparent)},
		.emissive = colour::to_linear(RgbaU8{m_data.emissive_factor, colour::max_channel_u8}),
	};
	m_ubo->set_data(&mat, sizeof(mat));
}
} // namespace levk::material
