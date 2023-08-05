#include <glm/gtx/transform.hpp>
#include <le/scene/ui/rect_transform.hpp>

namespace le::ui {
auto RectTransform::matrix() const -> glm::mat4 {
	return glm::mat4{
		glm::vec4{scale.x, 0.0f, 0.0f, 0.0f},
		glm::vec4{0.0f, scale.y, 0.0f, 0.0f},
		glm::vec4{0.0, 0.0, 1.0f, 0.0f},
		glm::vec4{position.x, position.y, 0.0f, 1.0f},
	};
}
} // namespace le::ui
