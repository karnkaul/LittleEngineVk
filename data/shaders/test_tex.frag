#version 450 core

struct Albedo {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

layout(location = 0) in vec4 fragColour;

layout(set = 1, binding = 1) uniform Material {
	Albedo albedo;
};

layout(set = 2, binding = 0) uniform sampler2D diffuse;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec4 outColour;

void main() {
	outColour = fragColour * texture(diffuse, uv);
	// outColour = vec4(albedo.diffuse, 1.0);
}
