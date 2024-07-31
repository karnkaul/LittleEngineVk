#pragma once
#include <levk/colour.hpp>
#include <levk/transform.hpp>

namespace levk {
/// \brief Instanced rendering.
struct RenderInstance {
	struct Baked;

	Transform transform{};
	RgbaU8 tint{colour::white_v};

	[[nodiscard]] auto bake(glm::mat4 const& parent = identity_mat_v) const -> Baked;
};

/// \brief Default RenderInstance.
inline constexpr auto render_instance_v = RenderInstance{};

/// \brief Baked RenderInstance.
struct RenderInstance::Baked {
	glm::mat4 model{1.0f};
	glm::mat4 normal{1.0f};
	glm::vec4 tint{1.0f};
};
} // namespace levk
