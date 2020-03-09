#version 450

layout (location = 0) in vec3 fragColour;
layout (location = 0) out vec4 outColour;

void main()
{
	outColour = vec4(fragColour, 1.0);
}
