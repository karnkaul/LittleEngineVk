#version 450

const uint MAX_LOCALS = 1024;
const uint MAX_TEXTURES = 1024;

const uint LOCAL_ID = 0;
const uint DIFFUSE_ID = 1;
const uint SPECULAR_ID = 2;

const uint TEXTURED = 1 << 0;
const uint SPECULAR = 1 << 1;

layout(std140, set = 0, binding = 0) uniform View
{
	mat4 mat_vp;
	mat4 mat_v;
	mat4 mat_p;
	mat4 mat_ui;
	vec3 pos_v;
} view;

layout(std140, set = 0, binding = 1) buffer readonly Locals
{
	mat4 mat_m;
	mat4 mat_n;
	vec4 tint;
	int flags;
} locals[MAX_LOCALS];

layout(set = 0, binding = 2) uniform sampler2D diffuse[MAX_TEXTURES];
layout(set = 0, binding = 3) uniform sampler2D specular[MAX_TEXTURES];

layout(push_constant) uniform Push
{
	uint ids[3];
} pc; 

layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 outColour;

void main()
{
	outColour = vec4(fragColour, 1.0);
	if ((locals[pc.ids[LOCAL_ID]].flags & TEXTURED) != 0)
	{
		outColour = texture(diffuse[pc.ids[DIFFUSE_ID]], texCoord);
		if ((locals[pc.ids[LOCAL_ID]].flags & SPECULAR) != 0)
		{
			outColour *= texture(specular[pc.ids[SPECULAR_ID]], texCoord);
		}
	}
	else
	{
		outColour = vec4(fragColour, 1.0);
	}
	outColour *= locals[pc.ids[LOCAL_ID]].tint;
}
