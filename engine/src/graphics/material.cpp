#include <spaced/engine/graphics/descriptor_updater.hpp>
#include <spaced/engine/graphics/material.hpp>

namespace spaced::graphics {
auto Material::or_default(Ptr<Material const> material) -> Material const& {
	static auto const ret = UnlitMaterial{};
	return material != nullptr ? *material : ret;
}

auto UnlitMaterial::bind_set(vk::CommandBuffer const cmd) const -> void {
	auto const& layout = PipelineCache::self().shader_layout().material;
	DescriptorUpdater{layout.set}.update_texture(layout.textures[0], Fallback::self().or_white(texture)).bind_set(cmd);
}

auto LitMaterial::bind_set(vk::CommandBuffer const cmd) const -> void {
	auto const& layout = PipelineCache::self().shader_layout().material;
	auto const data = Std140{
		.albedo = Rgba::to_linear(albedo.to_vec4()),
		.m_r_aco_am = {metallic, roughness, 0.0f, std::bit_cast<float>(alpha_mode)},
		.emissive = Rgba::to_linear({emissive_factor, 1.0f}),
	};
	DescriptorUpdater{layout.set}
		.write_uniform(layout.data, &data, sizeof(data))
		.update_texture(layout.textures[0], Fallback::self().or_white(base_colour))
		.update_texture(layout.textures[1], Fallback::self().or_white(metallic_roughness))
		.update_texture(layout.textures[2], Fallback::self().or_black(emissive))
		.bind_set(cmd);
}
} // namespace spaced::graphics
