#pragma once
#include <spaced/engine/core/nvec3.hpp>
#include <spaced/engine/core/radians.hpp>
#include <spaced/engine/transform.hpp>
#include <string>
#include <variant>

namespace spaced {
///
/// \brief The view / z plane for Camera.
///
struct ViewPlane {
	float near{};
	float far{};
};

///
/// \brief Models a 3D camera facing its -Z, with either perspective or orthographic projection parameters.
///
struct Camera {
	enum class Face {
		eNegativeZ,
		ePositiveZ,
	};

	struct Perspective {
		ViewPlane view_plane{0.1f, 1000.0f};
		Radians field_of_view{Degrees{75.0f}};
	};

	struct Orthographic {
		ViewPlane view_plane{-100.0f, 100.0f};
		glm::vec2 fixed_view{};
	};

	static auto orthographic(glm::vec2 extent, ViewPlane view_plane = {-100.0f, 100.0f}) -> glm::mat4;

	auto view() const -> glm::mat4;
	auto projection(glm::vec2 extent) const -> glm::mat4;

	std::string name{};
	Transform transform{};
	std::variant<Perspective, Orthographic> type{Perspective{}};
	float exposure{2.0f};
	Face face{Face::eNegativeZ};
};
} // namespace spaced
