struct Instance {
	mat4 model;
	mat4 normal;
	vec4 tint;
};

struct DirLight {
	vec4 rgb_intensity;
	vec3 direction;
};

struct Material {
	vec4 albedo;
	vec4 m_r_aco_am;
	vec4 emissive;
};
