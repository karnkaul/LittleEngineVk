#version 450

const uint TEXTURED = 1 << 0;

layout(set = 1, binding = 0) uniform Flags
{
	vec4 tint;
	int bits;
} flags;

layout(set = 1, binding = 1) uniform sampler2D diffuse;

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
	outColour *= flags.tint;
}
