#version 450 core

layout(set = 0, binding = 0) uniform VP {
	mat4 mat_p;
	mat4 mat_v;
	mat4 mat_ui;
};

layout(set = 1, binding = 0) uniform M {
	mat4 mat_m;
};

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertColour;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texCoord;

layout(location = 0) out vec4 fragColour;

vec2 pos[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	// gl_Position = mat_p * mat_v * mat_m * vec4(pos[gl_VertexIndex], 0.0, 1.0);
	gl_Position = mat_p * mat_v * mat_m * vec4(vertPos, 1.0);
	fragColour = vec4(1.0, 0.0, 0.0, 1.0);
}
