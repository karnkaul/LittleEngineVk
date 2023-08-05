#version 450 core

layout (set = 1, binding = 1) uniform sampler2D tex;

layout (location = 0) in vec4 in_rgba;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec4 in_frag_pos;

layout (location = 0) out vec4 out_rgba;

void main() {
	out_rgba = in_rgba * texture(tex, in_uv);
	if (out_rgba.w <= 0.0) { discard; }
}
