#pragma once
#include <glm/gtc/quaternion.hpp>
#include <le/core/nvec3.hpp>
#include <le/core/radians.hpp>
#include <le/core/transform.hpp>
#include <le/graphics/camera.hpp>
#include <le/graphics/rgba.hpp>
#include <le/imcpp/common.hpp>

namespace le::imcpp {
///
/// \brief Stateless ImGui helper to reflect various properties.
///
class Reflector {
  public:
	struct AsRgb {
		glm::vec3& out;
	};

	static constexpr auto min_v{-std::numeric_limits<float>::max()};
	static constexpr auto max_v{std::numeric_limits<float>::max()};

	///
	/// \brief Construct an Reflector instance.
	///
	/// Reflectors don't do anything on construction, constructors exist to enforce invariants instance-wide.
	/// For all Reflectors, an existing Window target is required, Reflector instances will not create any
	///
	Reflector(NotClosed<Window> /*window*/) {}

	auto operator()(char const* label, glm::vec2& out_vec2, float speed = 1.0f, float lo = min_v, float hi = max_v) const -> bool;
	auto operator()(char const* label, glm::vec3& out_vec3, float speed = 1.0f, float lo = min_v, float hi = max_v) const -> bool;
	auto operator()(char const* label, glm::vec4& out_vec4, float speed = 1.0f, float lo = min_v, float hi = max_v) const -> bool;
	auto operator()(char const* label, nvec3& out_vec3, float speed = 0.01f) const -> bool;
	auto operator()(char const* label, glm::quat& out_quat) const -> bool;
	auto operator()(char const* label, Radians& out_as_degrees, float speed = 0.05f, float lo = min_v, float hi = max_v) const -> bool;
	auto operator()(char const* label, AsRgb out_rgb) const -> bool;
	auto operator()(graphics::Rgba& out_rgba, bool show_alpha) const -> bool;
	auto operator()(graphics::HdrRgba& out_rgba, bool show_alpha, bool show_intensity = true) const -> bool;
	auto operator()(Transform& out_transform, bool& out_unified_scaling, bool scaling_toggle) const -> bool;
	auto operator()(graphics::ViewPlane& view_plane) const -> bool;
	auto operator()(graphics::Camera::Orthographic& orthographic) const -> bool;
	auto operator()(graphics::Camera::Perspective& perspective) const -> bool;
	auto operator()(graphics::Camera& out_camera) const -> bool;
};
} // namespace le::imcpp
