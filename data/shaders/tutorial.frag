#version 450 core

const uint MAX_TEXTURES = 1024;

const uint eTEXTURED = 1 << 0;
const uint eLIT = 1 << 1;

struct Material
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

struct DirLight
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 direction;
};

layout(std140, set = 0, binding = 0) uniform View
{
	mat4 mat_vp;
	mat4 mat_v;
	mat4 mat_p;
	mat4 mat_ui;
	vec3 pos_v;
};

layout(std430, set = 0, binding = 3) buffer readonly Materials
{
	Material materials[];
};

layout(std430, set = 0, binding = 4) buffer readonly Tints
{
	vec4 tints[];
};

layout(std430, set = 0, binding = 5) buffer readonly Flags
{
	uint flags[];
};

layout(set = 0, binding = 6) uniform sampler2D diffuse[MAX_TEXTURES];
layout(set = 0, binding = 7) uniform sampler2D specular[MAX_TEXTURES];

layout(push_constant) uniform Push
{
	uint objectID;
	uint diffuseID;
	uint specularID;
};

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragColour;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texCoord;

layout(location = 0) out vec4 outColour;

void main()
{
	outColour = vec4(fragColour, 1.0);
	vec4 ambientColour = materials[objectID].ambient;
	vec4 diffuseColour = materials[objectID].diffuse;
	vec4 specularColour = materials[objectID].specular;
	DirLight dirLight;
	dirLight.ambient = vec4(0.1);
	dirLight.diffuse = vec4(0.7);
	dirLight.specular = vec4(1.0);
	dirLight.direction = vec4(normalize(vec3(-0.0, -1.0, -1.0)), 1.0);
	if ((flags[objectID] & eTEXTURED) != 0)
	{
		ambientColour *= texture(diffuse[diffuseID], texCoord);
		diffuseColour *= texture(diffuse[diffuseID], texCoord);
		specularColour *= texture(specular[specularID], texCoord);
	}
	else
	{
		vec4 colour = vec4(fragColour, 1.0);
		ambientColour *= colour;
		diffuseColour *= colour;
		specularColour = vec4(0.0);
	}
	if ((flags[objectID] & eLIT) != 0)
	{
		vec3 toView = normalize(pos_v - fragPos);
		vec3 reflectDir = reflect(vec3(dirLight.direction), normal);
		float lambert = max(dot(vec3(-dirLight.direction), normal), 0.0);
		float phong = pow(max(dot(reflectDir, toView), 0.0), materials[objectID].shininess);
		ambientColour = vec4(vec3(dirLight.ambient), 1.0) * ambientColour;
		diffuseColour = vec4(vec3(dirLight.diffuse) * lambert, 1.0) * diffuseColour;
		specularColour = vec4(vec3(dirLight.specular) * phong, 1.0) * vec4(1.0) * specularColour;
	}
	outColour = max(ambientColour, 0.0) + max(diffuseColour, 0.0) + max(specularColour, 0.0);
	if (outColour.a < 0.1)
	{
		discard;
	}
	outColour *= tints[objectID];
}
