#version 450 core

layout(location = 0) in vec3 fragPos;

layout(set = 2, binding = 0) uniform samplerCube cubemap;

layout(location = 0) out vec4 outColour;

void main() {
	outColour = texture(cubemap, fragPos);
}
