#version 450 core

#include "attributes.shader"
#include "structs.shader"

layout (set = 0, binding = 0) uniform ViewProj {
	mat4 view_proj;
};

layout (set = 1, binding = 0) readonly buffer Instances {
	Instance instances[];
};

layout (location = 0) out vec4 out_tint;
layout (location = 1) out vec2 out_uv;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	const Instance instance = instances[gl_InstanceIndex];
	const vec4 frag_pos = instance.model * vec4(a_pos, 1.0);
	out_tint = a_rgba * instance.tint;
	out_uv = a_uv;
	gl_Position = view_proj * frag_pos;
}
