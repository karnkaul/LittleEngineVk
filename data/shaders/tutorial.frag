#version 450

layout(binding = 1) uniform Flags
{
	int isTextured;
} flags;

layout (location = 0) in vec3 fragColour;
layout (location = 0) out vec4 outColour;

void main()
{
	outColour = vec4(fragColour, 1.0);
	outColour = flags.isTextured == 0 ? vec4(1.0) : vec4(fragColour, 1.0);
}
