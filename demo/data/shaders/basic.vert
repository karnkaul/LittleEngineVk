#version 450 core

layout(std140, set = 0, binding = 0) uniform VP {
	mat4 mat_v;
	mat4 mat_p;
};

layout(std140, set = 1, binding = 0) uniform M {
	mat4 mat_m;
};

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec4 vertColour;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texCoord;

layout(location = 0) out vec4 fragColour;
layout(location = 1) out vec2 uv;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	fragColour = vertColour;
	uv = texCoord;
	gl_Position = mat_p * mat_v * mat_m * vec4(vertPos, 1.0);
}
