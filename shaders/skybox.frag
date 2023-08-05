#version 450 core

layout (set = 1, binding = 1) uniform samplerCube tex;

layout (location = 0) in vec4 in_rgba;
layout (location = 2) in vec4 in_frag_pos;

layout (location = 0) out vec4 out_rgba;

void main() {
	out_rgba = in_rgba * texture(tex, vec3(in_frag_pos));
}
