const float pi_v = 3.14159;

vec3 schlick_fresnel(float VdotH, float metallic) {
	const vec3 f0 = mix(vec3(0.04), vec3(material.albedo), metallic);
	const vec3 ret = f0 + (1 - f0) * pow(clamp(1 - VdotH, 0, 1), 5);
	return ret;
}

float ggx_distribution(float NdotH, float roughness) {
	const float alpha2 = roughness * roughness;
	const float d = NdotH * NdotH * (alpha2 - 1) + 1;
	return alpha2 / (pi_v * d * d);
}

float geometry_smith(float NdotL, float roughness) {
	const float k = (roughness + 1) * (roughness + 1) / 8;
	const float denom = NdotL * (1 - k) + k;
	return NdotL / denom;
}

struct Light {
	vec3 tint;
	float intensity;
};

// https://www.youtube.com/watch?v=XK_p2MxGBQs
vec3 pbr_brdf(Light light, vec3 albedo, float m, float r, vec3 l, vec3 n, vec3 v, vec3 h) {
	const float NdotH = max(dot(n, h), 0);
	const float VdotH = max(dot(v, h), 0);
	const float NdotL = max(dot(n, l), 0);
	const float NdotV = max(dot(n, v), 0);

	const vec3 F = schlick_fresnel(VdotH, m);
	const vec3 kS = F;
	const vec3 kD = 1 - kS;

	const vec3 spec_brdf_num = ggx_distribution(NdotH, r) * F * geometry_smith(NdotL, r) * geometry_smith(NdotV, r);
	const float spec_brdf_denom = 4 * NdotV * NdotL + 0.0001;
	const vec3 spec_brdf = spec_brdf_num / spec_brdf_denom;

	const vec3 f_lambert = mix(albedo.xyz, vec3(0), m);
	const vec3 diffuse_brdf = kD * f_lambert / pi_v;

	const vec3 total_brdf = diffuse_brdf + spec_brdf;
	vec3 ret = total_brdf * light.tint * light.intensity * in_campos_exposure.w * NdotL;
	ret /= (ret + 1);
	ret = max(ret, 0.03 * albedo.xyz);

	return ret;
}


float compute_visibility(sampler2D smap, vec3 shadow_dir, vec4 fpos_shadow, vec3 normal) {
	vec3 projected = fpos_shadow.xyz / fpos_shadow.w;
	if (projected.z > 1) {
		return 1;
	}
	// float bias = max(0.05 * (1.0 - dot(in_normal, -shadow_dir)), 0.005);
	const float slope = tan(acos(max(dot(normal, -shadow_dir), 0.0)));
	const float bias = clamp(0.005 * slope, 0.001, 0.05);
	const float current_depth = projected.z - bias;
	projected = projected * 0.5 + 0.5;
	projected.y = 1.0 - projected.y;
	float ret = 1.0;
	const vec2 texel_size = 1.0 / textureSize(smap, 0);
	for (int x = -1; x <= 1; ++x) {
		for (int y = -1; y <= 1; ++y) {
			float pcf_depth = texture(smap, projected.xy + vec2(x, y) * texel_size).x;
			float shadow = current_depth > pcf_depth ? 0.1 : 0.0;
			ret -= shadow;
		}
	}
	return max(ret, 0.1);
}
