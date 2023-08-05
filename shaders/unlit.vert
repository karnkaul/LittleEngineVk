#version 450 core

struct Instance {
	mat4 transform;
	vec4 tint;
};

layout (location = 0) in vec3 vpos;
layout (location = 1) in vec4 vrgba;
layout (location = 2) in vec3 vnormal;
layout (location = 3) in vec2 vuv;

layout (set = 0, binding = 0) uniform View {
	mat4 view;
	mat4 projection;
};

layout (set = 2, binding = 0) readonly buffer Instances {
	Instance instances[];
};

layout (location = 0) out vec4 out_rgba;
layout (location = 1) out vec2 out_uv;
layout (location = 2) out vec4 out_frag_pos;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	const Instance instance = instances[gl_InstanceIndex];
	out_frag_pos = instance.transform * vec4(vpos, 1.0);
	gl_Position = projection * view * out_frag_pos;

	out_rgba = vrgba * instance.tint;
	out_uv = vuv;
}
