#version 450 core

struct DirLight {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 direction;
};

layout(std140, set = 0, binding = 1) uniform DirLights {
	DirLight lights[4];
	uint count;
} dirLight;

layout(set = 2, binding = 0) uniform sampler2D diffuse;
layout(set = 2, binding = 1) uniform sampler2D rmo;
layout(set = 2, binding = 2) uniform sampler2D specular;

layout(std140, set = 3, binding = 0) uniform Material {
	vec4 tint;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
} material;

layout(location = 0) in vec4 fragColour;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 fragPos;
layout(location = 3) in vec3 viewPos;
layout(location = 4) in vec3 fragNorm;

layout(location = 0) out vec4 outColour;

vec4 opaque(vec4 vin) {
	return vec4(vec3(vin), 1.0);
}

void main() {
	const vec4 rmoParams = texture(rmo, uv);
	const float metallic = rmoParams.y;
	const float opacity = rmoParams.z;
	if (opacity < 0.1) {
		discard;
	}
	vec4 ambientColour = texture(diffuse, uv) * material.ambient;
	vec4 diffuseColour = texture(diffuse, uv) * material.diffuse;
	vec4 specularColour = texture(specular, uv) * vec4(vec3(material.specular), 1.0);
	vec4 ambientLight = vec4(0.0);
	vec4 diffuseLight = vec4(0.0);
	vec4 specularLight = vec4(0.0);
	for (uint i = 0; i < dirLight.count; ++i) {
		DirLight light = dirLight.lights[i];
		const vec3 direction = vec3(light.direction);
		const vec3 toView = normalize(viewPos - vec3(fragPos));
		const vec3 reflectDir = reflect(direction, fragNorm);
		const float lambert = max(dot(-direction, fragNorm), 0.0);
		const float phong = pow(max(dot(reflectDir, toView), 0.0), material.specular.w);
		ambientLight += opaque(light.ambient);
		diffuseLight += opaque(light.diffuse * lambert);
		specularLight += opaque(light.specular * phong * metallic);
	}
	ambientColour *= ambientLight;
	diffuseColour *= diffuseLight;
	specularColour *= specularLight;
	const vec4 litColour = min(max(ambientColour, 0.0) + max(diffuseColour, 0.0) + max(specularColour, 0.0), 1.0);
	outColour = fragColour * vec4(vec3(litColour), opacity) * material.tint;
}
