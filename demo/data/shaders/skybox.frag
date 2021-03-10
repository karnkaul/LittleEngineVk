#version 450 core

layout(location = 0) in vec3 fragPos;

layout(set = 0, binding = 1) uniform samplerCube cubemap;

layout(location = 0) out vec4 outColour;

void main() {
	outColour = texture(cubemap, fragPos);
}
