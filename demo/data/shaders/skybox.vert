#version 450 core

layout(std140, set = 0, binding = 0) uniform VP {
	mat4 mat_v;
	mat4 mat_p;
};

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertColour;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texCoord;

layout(location = 0) out vec3 fragPos;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec4 pos = vec4(vertPos, 1.0);
	gl_Position = mat_p * mat4(mat3(mat_v)) * pos;
	fragPos = vec3(mat4(1.0) * pos);
}
