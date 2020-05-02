#version 450 core

layout(std140, set = 0, binding = 0) uniform View
{
	mat4 mat_vp;
	mat4 mat_v;
	mat4 mat_p;
	mat4 mat_ui;
	vec3 pos_v;
	uint dirLightCount;
};

layout(std430, set = 0, binding = 1) buffer readonly Models
{
	mat4 mats_m[];
};

layout(std430, set = 0, binding = 2) buffer readonly Normals
{
	mat4 mats_n[];
};

layout(push_constant) uniform Push
{
	uint objectID;
};

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertColour;
layout(location = 2) in vec3 vertNormal;
layout(location = 3) in vec2 vertTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragColour;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragTexCoord;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	vec4 pos = vec4(vertPos, 1.0);
	vec4 mPos = mats_m[objectID] * pos;
	gl_Position = mat_vp * mPos;
	fragPos = vec3(mats_m[objectID] * pos);
	fragColour = vertColour;
	fragNormal = normalize(vec3(mats_n[objectID] * vec4(vertNormal, 1.0)));
	fragNormal = normalize(mat3(mats_n[objectID]) * vertNormal);
	fragTexCoord = vertTexCoord;
}
