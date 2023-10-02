#version 450 core

struct DirLight {
	vec3 direction;
	vec3 diffuse;
	vec3 ambient;
};

struct Instance {
	mat4 transform;
	vec4 tint;
};

layout (location = 0) in vec3 vpos;
layout (location = 1) in vec4 vrgba;
layout (location = 2) in vec3 vnormal;
layout (location = 3) in vec2 vuv;

layout (location = 4) in uvec4 joint;
layout (location = 5) in vec4 weight;

layout (set = 0, binding = 0) uniform View {
	mat4 view;
	mat4 projection;
	vec4 vpos_exposure;
	vec4 vdir_ortho;
	mat4 mat_shadow;
	vec4 shadow_dir;
};

layout (set = 0, binding = 1) readonly buffer DirLights {
	DirLight dir_lights[];
};

layout (set = 2, binding = 0) readonly buffer Instances {
	Instance instances[];
};

layout (set = 2, binding = 1) readonly buffer JointMats {
	mat4 mat_joints[];
};

layout (location = 0) out vec4 out_rgba;
layout (location = 1) out vec2 out_uv;
layout (location = 2) out vec4 out_frag_pos;
layout (location = 3) out vec3 out_normal;
layout (location = 4) out vec4 out_fpos_shadow;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	mat4 skin_mat =
		weight.x * mat_joints[joint.x] +
		weight.y * mat_joints[joint.y] +
		weight.z * mat_joints[joint.z] +
		weight.w * mat_joints[joint.w];

	const Instance instance = instances[gl_InstanceIndex];
	out_frag_pos = instance.transform * skin_mat * vec4(vpos, 1.0);
	gl_Position = projection * view * out_frag_pos;

	out_rgba = vrgba * instance.tint;
	out_uv = vuv;
	out_normal = normalize(vec3(skin_mat * vec4(vnormal, 0.0)));
	out_fpos_shadow = mat_shadow * out_frag_pos;
}
