#pragma once
#include <glm/mat4x4.hpp>

namespace le {
namespace graphics {
struct Camera;
}
struct Material;

struct ShaderSceneView {
	alignas(16) glm::mat4 mat_view;
	alignas(16) glm::mat4 mat_perspective;
	alignas(16) glm::mat4 mat_orthographic;
	alignas(16) glm::vec4 pos_camera;

	static ShaderSceneView make(graphics::Camera const& camera, glm::vec2 extent) noexcept;
};

struct ShaderMaterial {
	alignas(16) glm::vec4 tint;
	alignas(16) glm::vec4 ambient;
	alignas(16) glm::vec4 diffuse;
	alignas(16) glm::vec4 specular;

	static ShaderMaterial make(Material const& mtl) noexcept;
};
} // namespace le
