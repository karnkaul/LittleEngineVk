#version 450 core
#extension GL_EXT_nonuniform_qualifier : enable 

const uint MAX_LOCALS = 1024;

layout(std140, set = 0, binding = 0) uniform View
{
	mat4 mat_vp;
	mat4 mat_v;
	mat4 mat_p;
	mat4 mat_ui;
	vec3 pos_v;
} view;

layout(std430, set = 0, binding = 1) buffer readonly Locals
{
	mat4 mat_m;
	mat4 mat_n;
	vec4 tint;
	uint flags;
} locals[MAX_LOCALS];

layout(push_constant) uniform Push
{
	uint localID;
};

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertColour;
layout(location = 2) in vec2 vertTexCoord;

layout(location = 0) out vec3 fragColour;
layout(location = 1) out vec2 fragTexCoord;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	gl_Position = view.mat_vp * locals[localID].mat_m * vec4(vertPos, 1.0);
	fragColour = vertColour;
	fragTexCoord = vertTexCoord;
}
