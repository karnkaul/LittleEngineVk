#version 450 core

struct Albedo {
	vec4 colour;
	vec4 amdispsh;
};

struct DirLight {
	Albedo albedo;
	vec4 direction;
};

layout(std140, set = 0, binding = 1) uniform DirLights {
	DirLight lights[4];
	uint count;
} dirLight;

layout(std140, set = 1, binding = 1) uniform Material {
	Albedo albedo;
	uint flags;
} material;

const uint eDrop = 1 << 0;

layout(set = 2, binding = 0) uniform sampler2D diffuse;
layout(set = 2, binding = 1) uniform sampler2D specular;
layout(location = 1) in vec2 uv;

layout(location = 0) in vec4 fragColour;
layout(location = 2) in vec4 fragPos;
layout(location = 3) in vec3 viewPos;
layout(location = 4) in vec3 fragNorm;
layout(location = 0) out vec4 outColour;

void main() {
	vec4 ambientColour = texture(diffuse, uv);
	if ((material.flags & eDrop) != 0 && ambientColour == vec4(0.0)) {
		discard;
	}
	ambientColour *= material.albedo.colour * material.albedo.amdispsh.x;
	vec4 diffuseColour = texture(diffuse, uv) * material.albedo.colour * material.albedo.amdispsh.y;
	vec4 specularColour = texture(specular, uv) * material.albedo.colour * material.albedo.amdispsh.z;
	vec4 ambientLight = vec4(0.0);
	vec4 diffuseLight = vec4(0.0);
	vec4 specularLight = vec4(0.0);
	for (uint i = 0; i < dirLight.count; ++i) {
		DirLight dirLight = dirLight.lights[i];
		const vec3 colour = vec3(dirLight.albedo.colour);
		const vec3 direction = vec3(dirLight.direction);
		vec3 toView = normalize(viewPos - vec3(fragPos));
		vec3 reflectDir = reflect(direction, fragNorm);
		float lambert = max(dot(-direction, fragNorm), 0.0);
		float phong = pow(max(dot(reflectDir, toView), 0.0), material.albedo.amdispsh.w);
		ambientLight += vec4(dirLight.albedo.amdispsh.x * colour, 1.0);
		diffuseLight += vec4(dirLight.albedo.amdispsh.y * lambert * colour, 1.0);
		specularLight += vec4(dirLight.albedo.amdispsh.z * phong * colour, 1.0);
	}
	ambientColour *= ambientLight;
	diffuseColour *= diffuseLight;
	specularColour *= specularLight;
	vec4 litColour = min(max(ambientColour, 0.0) + max(diffuseColour, 0.0) + max(specularColour, 0.0), 1.0);
	outColour = fragColour * litColour;
}
