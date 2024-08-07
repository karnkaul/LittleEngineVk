#version 450 core

layout (set = 2, binding = 0) uniform samplerCube tex_cubemap;

layout (location = 0) in vec3 in_uv;

layout (location = 0) out vec4 out_rgba;

void main() {
	out_rgba = texture(tex_cubemap, in_uv);
}
