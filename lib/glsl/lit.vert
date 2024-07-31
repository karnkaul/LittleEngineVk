#version 450 core

#include "attributes.shader"
#include "structs.shader"

layout (set = 0, binding = 0) uniform ViewProj {
	mat4 view_proj;
	vec4 campos_exposure;
	mat4 shadow_view_proj;
};

layout (set = 1, binding = 0) readonly buffer Instances {
	Instance instances[];
};

layout (location = 0) out vec4 out_tint;
layout (location = 1) out vec2 out_uv;
layout (location = 2) out vec3 out_normal;
layout (location = 3) out vec4 out_fpos;
layout (location = 4) out vec4 out_campos_exposure;
layout (location = 5) out vec4 out_fpos_shadow;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	const Instance instance = instances[gl_InstanceIndex];
	const vec4 frag_pos = instance.model * vec4(a_pos, 1.0);
	out_tint = a_rgba * instance.tint;
	out_uv = a_uv;
	out_normal = normalize(vec3(instance.normal * vec4(a_normal, 0.0)));
	out_campos_exposure = campos_exposure;
	out_fpos = frag_pos;
	out_fpos_shadow = shadow_view_proj * out_fpos;
	gl_Position = view_proj * out_fpos;
}
