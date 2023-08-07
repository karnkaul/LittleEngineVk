#version 450 core

struct DirLight {
	vec3 direction;
	vec3 diffuse;
	vec3 ambient;
};

const uint ALPHA_OPAQUE = 0;
const uint ALPHA_BLEND = 1;
const uint ALPHA_MASK = 2;

struct Material {
	vec4 albedo;
	vec4 m_r_aco_am;
	vec4 emissive;
};

layout (set = 0, binding = 1) readonly buffer DL {
	DirLight dir_lights[];
};

layout (set = 1, binding = 0) uniform M {
	Material material;
};

layout (set = 0, binding = 2) uniform sampler2D shadow_map;

layout (set = 1, binding = 1) uniform sampler2D base_colour;
layout (set = 1, binding = 2) uniform sampler2D roughness_metallic;
layout (set = 1, binding = 3) uniform sampler2D emissive;

layout (location = 0) in vec4 in_rgba;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec4 in_frag_pos;
layout (location = 3) in vec3 in_normal;
layout (location = 4) in vec4 in_vpos_exposure;
layout (location = 5) in vec3 in_vdir;
layout (location = 6) flat in uint in_is_ortho;
layout (location = 7) in vec4 in_fpos_shadow;
layout (location = 8) in vec3 in_shadow_dir;

layout (location = 0) out vec4 out_rgba;

const float pi_v = 3.14159;

float distribution_ggx(vec3 N, vec3 H, float roughness) {
	const float a = roughness * roughness;
	const float a2 = a * a;
	const float NdotH = max(dot(N, H), 0.0);
	const float NdotH2 = NdotH * NdotH;

	const float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = pi_v * denom * denom;

	return num / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness) {
	const float r = (roughness + 1.0);
	const float k = (r * r) / 8.0;

	const float num = NdotV;
	const float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}

float geometry_smith(float NdotV, float NdotL, float roughness) {
	const float ggx2 = geometry_schlick_ggx(NdotV, roughness);
	const float ggx1 = geometry_schlick_ggx(NdotL, roughness);
	return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cos, vec3 F0) { return F0 + (vec3(1.0) - F0) * pow(max(1.0 - cos, 0.0), 5.0); }

vec3 gamc(vec3 a) {
	const float exp = 1.0f / 2.2f;
	return vec3(
		pow(a.x, exp),
		pow(a.y, exp),
		pow(a.z, exp)
	);
}

vec3 cook_torrance() {
	const float roughness = material.m_r_aco_am.y * texture(roughness_metallic, in_uv).g;
	const float metallic = material.m_r_aco_am.x * texture(roughness_metallic, in_uv).b;
	const vec3 f0 = mix(vec3(0.04), vec3(material.albedo), metallic);

	vec3 L0 = vec3(0.0);
	const vec3 V = normalize(in_is_ortho == 1 ? in_vdir : in_vpos_exposure.xyz - in_frag_pos.xyz);
	const vec3 N = in_normal;
	for (int i = 0; i < dir_lights.length(); ++i) {
		DirLight light = dir_lights[i];
		const vec3 L = -light.direction;
		const vec3 H = normalize(V + L);

		const float NdotL = max(dot(N, L), 0.0);
		const float NdotV = max(dot(N, V), 0.0);

		const float NDF = distribution_ggx(N, H, roughness);
		const float G = geometry_smith(NdotV, NdotL, roughness);
		const vec3 F = fresnel_schlick(max(dot(H, V), 0.0), f0);

		const vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;

		const vec3 num = NDF * kS * G;
		const float denom = 4.0 * NdotV * NdotL + 0.0001;
		const vec3 spec = num / denom;

		L0 += (kD * vec3(material.albedo) / pi_v + spec) * light.diffuse * max(in_vpos_exposure.w, 0.0) * NdotL;
	}

	vec3 colour = max(L0, 0.03 * vec3(material.albedo));

	colour /= (colour + vec3(1.0));
	return colour;
}

float compute_visibility() {
	vec3 projected = in_fpos_shadow.xyz / in_fpos_shadow.w;
	if (projected.z > 1.0) {
		return 1.0;
	}
	// float bias = max(0.05 * (1.0 - dot(in_normal, -in_shadow_dir)), 0.005);
	float slope = tan(acos(max(dot(in_normal, -in_shadow_dir), 0.0)));
	float bias = clamp(0.005 * slope, 0.001, 0.05);
	float current_depth = projected.z - bias;
	projected = projected * 0.5 + 0.5;
	projected.y = 1.0 - projected.y;
	float ret = 1.0;
	vec2 texel_size = 1.0 / textureSize(shadow_map, 0);
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			float pcf_depth = texture(shadow_map, projected.xy + vec2(x, y) * texel_size).x;
			float shadow = current_depth > pcf_depth ? 0.1 : 0.0;
			ret -= shadow;
		}
	}
	return max(ret, 0.1);
}

void main() {
	vec4 diffuse = texture(base_colour, in_uv);
	const float alpha_cutoff = material.m_r_aco_am.z;
	const uint alpha_mode = floatBitsToUint(material.m_r_aco_am.w);
	if (alpha_mode == ALPHA_OPAQUE) {
		diffuse.w = 1.0;
	} else if (alpha_mode == ALPHA_MASK) {
		if (diffuse.w < alpha_cutoff) { discard; }
		diffuse.w = 1.0;
	}

	const float visibility = compute_visibility();
	out_rgba = (visibility * vec4(cook_torrance(), 1.0)) * vec4(vec3(in_rgba), 1.0) * diffuse + material.emissive * texture(emissive, in_uv);

	if (alpha_mode == ALPHA_BLEND && out_rgba.w <= 0.0) {
		discard;
	}
}
