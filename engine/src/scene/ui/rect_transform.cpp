#include <glm/gtx/transform.hpp>
#include <spaced/scene/ui/rect_transform.hpp>

namespace spaced::ui {
auto RectTransform::matrix() const -> glm::mat4 {
	auto const centre = frame.centre();
	auto const t = glm::mat4{
		glm::vec4{1.0f, 0.0f, 0.0f, 0.0f},
		glm::vec4{0.0f, 1.0f, 0.0f, 0.0f},
		glm::vec4{0.0, 0.0, 1.0f, 0.0f},
		glm::vec4{centre.x, centre.y, 0.0f, 1.0f},
	};
	auto const s = glm::mat4{
		glm::vec4{scale.x, 0.0f, 0.0f, 0.0f},
		glm::vec4{0.0f, scale.y, 0.0f, 0.0f},
		glm::vec4{0.0, 0.0, 1.0f, 0.0f},
		glm::vec4{0.0f, 0.0f, 0.0f, 1.0f},
	};
	return t * s;
}
} // namespace spaced::ui
