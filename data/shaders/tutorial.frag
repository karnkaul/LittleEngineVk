#version 450

const int TEXTURED = 1 << 0;

layout(binding = 1) uniform Flags
{
	int bits;
} flags;

layout(binding = 2) uniform sampler2D diffuse;

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 outColour;

void main()
{
	outColour = vec4(fragColour, 1.0);
	if ((flags.bits & TEXTURED) != 0)
	{
		outColour = texture(diffuse, texCoord);
	}
	else
	{
		outColour = vec4(fragColour, 1.0);
	}
}
