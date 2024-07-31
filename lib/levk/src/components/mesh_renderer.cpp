#include <levk/components/mesh_renderer.hpp>
#include <levk/core/fixed_string.hpp>
#include <levk/imcpp/im_text.hpp>

namespace levk {
namespace {
void inspect_primitives(PrimitivesView const primitives) {
	if (ImGui::TreeNode("primitives")) {
		for (auto const [index, primitive] : std::ranges::enumerate_view(primitives)) {
			auto const label = FixedString<>{"[{}]", index};
			if (ImGui::TreeNode(label.c_str())) {
				im_text("triangles: {}", primitive->get_triangle_count());
				im_text("material: {}", primitive->material->name);
				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}
}

void inspect_skeleton(Skeleton& skeleton) {
	if (ImGui::TreeNode("skeleton")) {
		im_text("{}", skeleton.name);
		if (skeleton.animation != nullptr) {
			im_text("animation: {}", skeleton.animation->name);
			im_text("elapsed: {:.1f}s", skeleton.elapsed.count());
			auto const duration = skeleton.animation->get_duration();
			auto progress = skeleton.elapsed / duration;
			if (ImGui::SliderFloat("progress", &progress, 0.0f, 1.0f, "%.2f")) { skeleton.elapsed = progress * duration; }
			if (ImGui::BeginCombo("animations", skeleton.animation->name.data())) {
				auto const& animations = skeleton.get_animations();
				for (auto const& animation : animations) {
					if (ImGui::Selectable(animation->name.c_str(), animation->name == skeleton.animation->name)) {
						skeleton.animation = animation;
						skeleton.elapsed = 0s;
					}
				}
				ImGui::EndCombo();
			}
		}
		ImGui::TreePop();
	}
}
} // namespace

void StaticMeshRenderer::im_inspect() {
	im_text("mesh: {}", mesh->name);
	inspect_primitives(mesh->get_primitives());
}

void SkinnedMeshRenderer::im_inspect() {
	im_text("mesh: {}", m_mesh->name);
	inspect_primitives(m_mesh->get_primitives());
	inspect_skeleton(m_skeleton);
}
} // namespace levk
