#pragma once
#include <imgui.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <levk/core/c_string.hpp>
#include <levk/core/fixed_string.hpp>
#include <levk/core/inclusive_range.hpp>
#include <optional>

namespace levk {
template <typename Type>
auto im_reset_small_btn(CString const label, Type& out, Type const& reset_value = Type{}) -> bool {
	auto const button_label = FixedString<128>{"reset##reset_{}", label.as_view()};
	if (ImGui::SmallButton(button_label.c_str())) {
		out = reset_value;
		return true;
	}
	return false;
}

struct ImDragFloat {
	static constexpr auto width_v{200.0f};

	float speed{0.25f};
	InclusiveRange<float> range{};
	CString format{"%.3f"};
	int flags{};

	std::optional<float> width{};
	bool reset_button{true};

	auto f32(CString const label, float& out, float const reset_value = 0.0f) const -> bool {
		if (width) { ImGui::SetNextItemWidth(*width); }
		auto ret = ImGui::DragFloat(label.c_str(), &out, speed, range.lo, range.hi, format.c_str(), flags);
		if (reset_button) {
			ImGui::SameLine();
			ret |= im_reset_small_btn(label, out, reset_value);
		}
		return ret;
	}

	template <glm::length_t Length>
	auto fvec(CString const label, glm::vec<Length, float>& out, glm::vec<Length, float> const& reset_value = {}, bool unified = false) const -> bool {
		if (width) { ImGui::SetNextItemWidth(*width); }
		auto* func = [unified = unified] {
			if (unified) { return &ImGui::DragFloat; }
			if constexpr (Length == 2) {
				return &ImGui::DragFloat2;
			} else if constexpr (Length == 3) {
				return &ImGui::DragFloat3;
			} else if constexpr (Length == 4) {
				return &ImGui::DragFloat4;
			} else {
				return &ImGui::DragFloat;
			}
		}();
		auto ret = func(label.c_str(), &out.x, speed, range.lo, range.hi, format.c_str(), flags);
		if (ret && unified) { out = glm::vec<Length, float>{out.x}; }
		if (reset_button) {
			ImGui::SameLine();
			ret |= im_reset_small_btn(label, out, reset_value);
		}
		return ret;
	}

	auto euler(CString const label, glm::quat& out) const -> bool {
		auto euler = glm::eulerAngles(out);
		euler = {glm::degrees(euler.x), glm::degrees(euler.y), glm::degrees(euler.z)};
		if (fvec(label, euler)) {
			out = glm::vec3{glm::radians(euler.x), glm::radians(euler.y), glm::radians(euler.z)};
			return true;
		}
		return false;
	}
};
} // namespace levk
