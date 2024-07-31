#version 450 core

layout (set = 2, binding = 0) uniform sampler2D tex_diffuse;

layout (location = 0) in vec4 in_tint;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec4 out_rgba;

void main() {
	out_rgba = texture(tex_diffuse, in_uv) * in_tint;
}
