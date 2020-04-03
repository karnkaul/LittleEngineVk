#version 450

layout(set = 0, binding = 0) uniform View
{
	mat4 mat_pv;
	mat4 mat_v;
} view;

layout(push_constant) uniform PushConsts
{
	mat4 mat_m;
} pushConsts;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColour;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColour;
layout(location = 1) out vec2 texCoord;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	gl_Position = view.mat_pv * pushConsts.mat_m * vec4(inPosition, 1.0);
	fragColour = inColour;
	texCoord = inTexCoord;
}
