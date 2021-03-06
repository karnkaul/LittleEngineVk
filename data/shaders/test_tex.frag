#version 450 core

layout(set = 2, binding = 0) uniform sampler2D diffuse;
layout(location = 1) in vec2 uv;

layout(location = 0) in vec4 fragColour;
layout(location = 0) out vec4 outColour;

void main() {
	outColour = fragColour * texture(diffuse, uv);
}
