#include <imgui.h>
#include <spaced/core/fixed_string.hpp>
#include <spaced/imcpp/reflector.hpp>
#include <optional>

namespace spaced::imcpp {
namespace {
constexpr auto to_degree(glm::vec3 const& angles) -> glm::vec3 { return {glm::degrees(angles.x), glm::degrees(angles.y), glm::degrees(angles.z)}; }

struct Modified {
	bool value{};

	constexpr auto operator()(bool const modified) -> bool {
		value |= modified;
		return modified;
	}
};

using DragFloatFunc = bool (*)(char const*, float* v, float, float, float, char const*, ImGuiSliderFlags);

// NOLINTNEXTLINE
constexpr DragFloatFunc drag_float_vtable[] = {
	nullptr, &ImGui::DragFloat, &ImGui::DragFloat2, &ImGui::DragFloat3, &ImGui::DragFloat4,
};

template <std::size_t Dim>
auto drag_float(char const* label, std::array<float, Dim>& data, float speed, float lo, float hi, int flags = {}) -> bool {
	static_assert(Dim > 0 && Dim < std::size(drag_float_vtable));
	return drag_float_vtable[Dim](label, data.data(), speed, lo, hi, "%.3f", flags);
}

template <std::size_t Dim>
auto reflect_vec(char const* label, glm::vec<static_cast<int>(Dim), float>& out_vec, float speed, float lo, float hi) -> bool {
	auto data = std::array<float, Dim>{};
	std::memcpy(data.data(), &out_vec, Dim * sizeof(float));
	if (drag_float(label, data, speed, lo, hi)) {
		std::memcpy(&out_vec, data.data(), Dim * sizeof(float));
		return true;
	}
	return false;
}
} // namespace

auto Reflector::operator()(char const* label, glm::vec2& out_vec2, float speed, float lo, float hi) const -> bool {
	return reflect_vec<2>(label, out_vec2, speed, lo, hi);
}

auto Reflector::operator()(char const* label, glm::vec3& out_vec3, float speed, float lo, float hi) const -> bool {
	return reflect_vec<3>(label, out_vec3, speed, lo, hi);
}

auto Reflector::operator()(char const* label, glm::vec4& out_vec4, float speed, float lo, float hi) const -> bool {
	return reflect_vec<4>(label, out_vec4, speed, lo, hi);
}

auto Reflector::operator()(char const* label, nvec3& out_vec3, float speed) const -> bool {
	auto vec3 = out_vec3.value();
	if ((*this)(label, vec3, speed, -1.0f, 1.0f)) {
		out_vec3 = vec3;
		return true;
	}
	return false;
}

auto Reflector::operator()(char const* label, glm::quat& out_quat) const -> bool {
	auto euler = to_degree(glm::eulerAngles(out_quat));
	auto deg = std::array<float, 3>{euler.x, euler.y, euler.z};
	auto const org = euler;
	bool ret = false;
	if (drag_float(label, deg, 0.5f, -180.0f, 180.0f, ImGuiSliderFlags_NoInput)) {
		euler = {deg[0], deg[1], deg[2]};
		if (auto const diff = org.x - euler.x; std::abs(diff) > 0.0f) { out_quat = glm::rotate(out_quat, glm::radians(diff), right_v); }
		if (auto const diff = org.y - euler.y; std::abs(diff) > 0.0f) { out_quat = glm::rotate(out_quat, glm::radians(diff), up_v); }
		if (auto const diff = org.z - euler.z; std::abs(diff) > 0.0f) { out_quat = glm::rotate(out_quat, glm::radians(diff), front_v); }
		ret = true;
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Reset")) {
		out_quat = quat_identity_v;
		ret = true;
	}
	return ret;
}

auto Reflector::operator()(char const* label, Radians& out_as_degrees, float speed, float lo, float hi) const -> bool {
	auto deg = Degrees{out_as_degrees};
	if (ImGui::DragFloat(label, &deg.value, speed, lo, hi)) {
		out_as_degrees = deg;
		return true;
	}
	return false;
}

auto Reflector::operator()(char const* label, AsRgb out_rgb) const -> bool {
	float arr[3] = {out_rgb.out.x, out_rgb.out.y, out_rgb.out.z};
	if (ImGui::ColorEdit3(label, arr)) {
		out_rgb.out = {arr[0], arr[1], arr[2]};
		return true;
	}
	return false;
}

auto Reflector::operator()(graphics::Rgba& out_rgba, bool show_alpha) const -> bool {
	auto hdr = graphics::HdrRgba{out_rgba, 1.0f};
	if ((*this)(hdr, show_alpha, false)) {
		out_rgba = hdr;
		return true;
	}
	return false;
}

auto Reflector::operator()(graphics::HdrRgba& out_rgba, bool show_alpha, bool show_intensity) const -> bool {
	auto ret = Modified{};
	auto vec4 = graphics::HdrRgba{out_rgba, 1.0f}.to_vec4();
	auto vec3 = glm::vec3{vec4};
	if ((*this)("RGB", AsRgb{vec3})) {
		out_rgba = graphics::HdrRgba::from(glm::vec4{vec3, vec4.w}, out_rgba.intensity);
		ret.value = true;
	}
	if (show_alpha) {
		if (ret(ImGui::DragFloat("Alpha", &vec4.w, 0.05f, 0.0f, 1.0f))) { out_rgba.channels[3] = graphics::Rgba::to_u8(vec4.w); }
	}
	if (show_intensity) { ret(ImGui::DragFloat("Intensity", &out_rgba.intensity, 0.05f, 0.1f, 1000.0f)); }
	return ret.value;
}

auto Reflector::operator()(Transform& out_transform, bool& out_unified_scaling, bool scaling_toggle) const -> bool {
	auto ret = Modified{};
	auto vec3 = out_transform.position();
	if (ret((*this)("Position", vec3, 0.01f))) { out_transform.set_position(vec3); }
	auto quat = out_transform.orientation();
	if (ret((*this)("Orientation", quat))) { out_transform.set_orientation(quat); }
	vec3 = out_transform.scale();
	if (out_unified_scaling) {
		if (ret(ImGui::DragFloat("Scale", &vec3.x, 0.01f))) { out_transform.set_scale({vec3.x, vec3.x, vec3.x}); }
	} else {
		if (ret((*this)("Scale", vec3, 0.01f))) { out_transform.set_scale(vec3); }
	}
	if (scaling_toggle) {
		ImGui::SameLine();
		ImGui::Checkbox("Unified", &out_unified_scaling);
	}
	return ret.value;
}

auto Reflector::operator()(graphics::ViewPlane& view_plane) const -> bool {
	auto ret = Modified{};
	if (auto tn = TreeNode{"ViewPlane"}) {
		ret(ImGui::DragFloat("Near", &view_plane.near));
		ret(ImGui::DragFloat("Far", &view_plane.far));
	}
	return ret.value;
}

auto Reflector::operator()(graphics::Camera::Orthographic& orthographic) const -> bool {
	auto ret = Modified{};
	ret((*this)("Fixed View", orthographic.fixed_view, 1.0f, 0.01f, 10000.0f));
	ret((*this)(orthographic.view_plane));
	return ret.value;
}

auto Reflector::operator()(graphics::Camera::Perspective& perspective) const -> bool {
	auto ret = Modified{};
	ret((*this)("FOV (Y)", perspective.field_of_view, 1.0f, 10.0f, 170.0f));
	ret((*this)(perspective.view_plane));
	return ret.value;
}

auto Reflector::operator()(graphics::Camera& out_camera) const -> bool {
	auto ret = Modified{};
	ret(ImGui::DragFloat("Exposure", &out_camera.exposure, 0.1f, 0.0f, 10.0f));
	auto uniform_scaling = bool{true};
	(*this)(out_camera.transform, uniform_scaling, {});
	std::visit([this, &ret](auto& payload) { ret((*this)(payload)); }, out_camera.type);
	return ret.value;
}
} // namespace spaced::imcpp
