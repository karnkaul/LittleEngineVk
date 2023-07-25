#pragma once
#include <spaced/engine/camera.hpp>
#include <spaced/engine/graphics/lights.hpp>
#include <vulkan/vulkan.hpp>

namespace spaced::graphics {
class RenderCamera {
  public:
	struct Std140View {
		glm::mat4 mat_vp;
		glm::vec4 vpos_exposure;
	};

	struct Std430DirLight {
		glm::vec4 direction;
		glm::vec4 diffuse;
		glm::vec4 ambient;
	};

	auto bind_set(glm::vec2 projection, vk::CommandBuffer cmd) const -> void;

	Camera camera{};
	Lights lights{};
};
} // namespace spaced::graphics
