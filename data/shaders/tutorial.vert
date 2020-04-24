#version 450 core

layout(std140, set = 0, binding = 0) uniform View
{
	mat4 mat_vp;
	mat4 mat_v;
	mat4 mat_p;
	mat4 mat_ui;
	vec3 pos_v;
};

layout(std430, set = 0, binding = 1) buffer readonly Models
{
	mat4 mats_m[];
};

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
	gl_Position = mat_vp * mats_m[localID] * vec4(vertPos, 1.0);
	fragColour = vertColour;
	fragTexCoord = vertTexCoord;
}
