#include <le/scene/ui/element.hpp>

namespace le::ui {
auto Element::local_matrix() const -> glm::mat4 {
	static constexpr auto identity_v{glm::identity<glm::mat4>()};
	auto const t = glm::translate(identity_v, glm::vec3{transform.position, z_index});
	auto const cos = glm::cos(rotation.value);
	auto const sin = glm::sin(rotation.value);
	auto const r = glm::mat4{
		glm::vec4{sin, cos, 0.0f, 0.0f},
		glm::vec4{cos, -sin, 0.0f, 0.0f},
		glm::vec4{0.0f, 0.0f, 1.0f, 0.0f},
		glm::vec4{0.0f, 0.0f, 0.0f, 1.0f},
	};
	auto const s = glm::scale(identity_v, glm::vec3{transform.scale, 1.0f});
	return t * r * s;
}
} // namespace le::ui
