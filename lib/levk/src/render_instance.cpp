#include <glm/gtc/matrix_inverse.hpp>
#include <levk/render_instance.hpp>

namespace levk {
auto RenderInstance::bake(glm::mat4 const& parent) const -> Baked {
	auto ret = Baked{
		.model = transform.to_matrix(parent),
		.tint = colour::to_linear(tint),
	};
	ret.normal = glm::inverseTranspose(glm::mat3{ret.model});
	return ret;
}
} // namespace levk
