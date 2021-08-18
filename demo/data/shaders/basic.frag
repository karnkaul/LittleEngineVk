#version 450 core

layout(location = 0) in vec4 fragColour;

layout(location = 1) in vec2 uv;

layout(location = 0) out vec4 outColour;

layout(std140, set = 3, binding = 0) uniform Material {
	vec4 tint;
} material;

void main() {
	outColour = fragColour * material.tint;
	//outColour = vec4(1.0, 0.0, 0.0, 1.0);
}
