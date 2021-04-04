#version 450 core

layout(location = 0) in vec4 fragColour;

layout(set = 2, binding = 0) uniform sampler2D diffuse;
layout(set = 2, binding = 1) uniform sampler2D rmo;

layout(location = 1) in vec2 uv;

layout(location = 0) out vec4 outColour;

void main() {
	const vec4 rmoParams = texture(rmo, uv);
	const float opacity = rmoParams.z;
	outColour = fragColour * texture(diffuse, uv) * opacity;
}
