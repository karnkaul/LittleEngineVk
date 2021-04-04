#version 450 core

layout(std140, set = 0, binding = 0) uniform VP {
	mat4 mat_v;
	mat4 mat_p;
	mat4 mat_ui;
	vec4 pos_v;
};

layout(std140, set = 1, binding = 0) uniform M {
	mat4 mat_m;
};

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertColour;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texCoord;

layout(location = 0) out vec4 fragColour;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec4 fragPos;
layout(location = 3) out vec3 viewPos;
layout(location = 4) out vec3 fragNorm;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	fragColour = vec4(vertColour, 1.0);
	uv = texCoord;
	fragPos = mat_m * vec4(vertPos, 1.0);
	viewPos = vec3(pos_v);
	fragNorm = normalize(mat3(mat_m) * normal);
	gl_Position = mat_p * mat_v * fragPos;
}
