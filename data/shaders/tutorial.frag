#version 450

layout(binding = 1) uniform Flags
{
	int isTextured;
} flags;

layout(binding = 2) uniform sampler2D diffuse;

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 outColour;

void main()
{
	outColour = vec4(fragColour, 1.0);
	outColour = flags.isTextured == 0 ? vec4(1.0) : vec4(fragColour, 1.0);
	outColour = flags.isTextured == 0 ? vec4(fragColour, 1.0) : texture(diffuse, texCoord);
}
