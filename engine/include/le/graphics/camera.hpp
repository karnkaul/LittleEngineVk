#pragma once
#include <le/core/nvec3.hpp>
#include <le/core/radians.hpp>
#include <le/core/transform.hpp>
#include <le/graphics/rgba.hpp>
#include <string>
#include <variant>

namespace le::graphics {
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

	struct Fog {
		Rgba tint{.channels = {0x77, 0x77, 0x77, 0xff}};
		float start{10.0f};
		float thickness{40.0f};
	};

	static auto orthographic(glm::vec2 extent, ViewPlane view_plane = {-100.0f, 100.0f}) -> glm::mat4;

	auto view() const -> glm::mat4;
	auto projection(glm::vec2 extent) const -> glm::mat4;

	std::string name{};
	Transform transform{};
	std::variant<Perspective, Orthographic> type{Perspective{}};
	float exposure{2.0f};
	Face face{Face::eNegativeZ};
	Rgba clear_colour{black_v};
	Fog fog{};
};
} // namespace le::graphics
