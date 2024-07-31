#version 450 core

#include "structs.shader"

layout (set = 0, binding = 1) uniform DL {
	DirLight main_light;
};

layout (set = 0, binding = 2) uniform sampler2D shadow_map;

layout (set = 2, binding = 0) uniform Mat {
	Material material;
};

layout (set = 2, binding = 1) uniform sampler2D tex_base_colour;
layout (set = 2, binding = 2) uniform sampler2D tex_metallic_roughness;
layout (set = 2, binding = 3) uniform sampler2D tex_emissive;
layout (set = 2, binding = 4) uniform sampler2D tex_normal;

layout (location = 0) in vec4 in_tint;
layout (location = 1) in vec2 in_uv;
layout (location = 3) in vec4 in_fpos;
layout (location = 4) in vec4 in_campos_exposure;
layout (location = 5) in vec4 in_fpos_shadow;
layout (location = 6) in mat3 in_tbn;

layout (location = 0) out vec4 out_rgba;

#include "pbr.shader"

void main() {
	vec4 diffuse = texture(tex_base_colour, in_uv);
	const float alpha_cutoff = material.m_r_aco_am.z;
	const uint is_transparent = floatBitsToUint(material.m_r_aco_am.w);
	if (is_transparent == 0) {
		diffuse.w = 1.0;
	} else {
		if (diffuse.w < alpha_cutoff) { discard; }
	}

	const float metallic = material.m_r_aco_am.x * texture(tex_metallic_roughness, in_uv).b;
	const float roughness = material.m_r_aco_am.y * texture(tex_metallic_roughness, in_uv).g;

	vec3 normal = texture(tex_normal, in_uv).xyz;
	normal = normalize(2.0 * normal - 1.0);
	const vec3 l = -main_light.direction;
	const vec3 n = normalize(in_tbn * normal);
	const vec3 v = normalize(in_campos_exposure.xyz - in_fpos.xyz);
	const vec3 h = normalize(n + v);
	const Light light = Light(main_light.rgb_intensity.xyz, main_light.rgb_intensity.w);
	const vec3 brdf_result = pbr_brdf(light, material.albedo.xyz, metallic, roughness, l, n, v, h);

	// const float visibility = compute_visibility();
	// const float visibility = 1.0;
	const float visibility = compute_visibility(shadow_map, main_light.direction, in_fpos_shadow, n);
	out_rgba = (visibility * vec4(brdf_result, 1.0)) * vec4(vec3(in_tint), 1.0) * diffuse + material.emissive * texture(tex_emissive, in_uv);
}
