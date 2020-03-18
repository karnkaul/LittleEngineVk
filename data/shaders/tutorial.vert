#version 450

layout(binding = 0) uniform Matrices
{
	mat4 _m;
	mat4 _pv;
	mat4 _v;
} mats;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColour;

layout (location = 0) out vec3 fragColour;

void main()
{
	gl_Position = mats._pv * mats._m * vec4(inPosition, 0.0, 1.0);
	fragColour = inColour;
}
