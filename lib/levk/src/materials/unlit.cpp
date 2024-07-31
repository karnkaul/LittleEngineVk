#include <levk/materials/unlit.hpp>

namespace levk::material {
Unlit::Unlit(IRenderDevice const& render_device, RenderShader const& fragment_shader)
	: IMaterial(fragment_shader), texture(&render_device.get_fallback_texture()) {}

void Unlit::push_descriptors(DescriptorBuffer& descriptor_buffer) const { descriptor_buffer.insert(texture->get_descriptor_info(bindings.texture)); }
} // namespace levk::material
