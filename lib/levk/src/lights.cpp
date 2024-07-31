#include <levk/imcpp/im_controls.hpp>
#include <levk/lights.hpp>

namespace levk {
void Light::inspect_light() {
	auto ftint = colour::to_f32(tint);
	if (ImGui::ColorEdit3("tint", &ftint.x)) { tint = colour::to_u8(ftint); }
	ImDragFloat{.range = {0.5f, 500.0f}}.f32("intensity", intensity, 1.0f);
}

void DirectionalLight::inspect_directional_light() {
	inspect_light();
	ImDragFloat{}.euler("direction", direction);
}
} // namespace levk
