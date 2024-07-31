#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace levk {
/// \brief A single vertex.
struct Vertex {
	glm::vec3 position{};
	glm::vec2 uv{};
	glm::vec4 rgba{1.0f};
	glm::vec3 normal{0.0f, 0.0f, 1.0f};
	glm::vec4 tangent{1.0f, 0.0f, 0.0f, 0.0f};
};

/// \brief Type of vertex input binding.
enum class VertexBinding : int { eDefault, eSkinned, eCOUNT_ };
} // namespace levk
