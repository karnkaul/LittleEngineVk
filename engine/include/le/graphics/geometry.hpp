#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <le/graphics/rect.hpp>
#include <le/graphics/rgba.hpp>
#include <span>
#include <vector>

namespace le::graphics {
struct Quad {
	glm::vec2 size{1.0f, 1.0f};
	UvRect uv{uv_rect_v};
	Rgba rgb{white_v};
	glm::vec3 origin{};

	auto operator==(Quad const&) const -> bool = default;
};

struct Cube {
	glm::vec3 size{1.0f, 1.0f, 1.0f};
	Rgba rgb{white_v};
	glm::vec3 origin{};

	auto operator==(Cube const&) const -> bool = default;
};

struct Sphere {
	static constexpr std::uint8_t resolution_v{16};

	float diameter{1.0f};
	std::uint32_t resolution{resolution_v};
	Rgba rgb{white_v};
	glm::vec3 origin{};

	auto operator==(Sphere const&) const -> bool = default;
};

struct Vertex {
	glm::vec3 position{};
	glm::vec4 rgba{1.0f};
	glm::vec3 normal{0.0f, 0.0f, 1.0f};
	glm::vec2 uv{};
};

struct Bone {
	glm::uvec4 joint{};
	glm::vec4 weight{};
};

struct Geometry {
	std::vector<Vertex> vertices{};
	std::vector<std::uint32_t> indices{};
	std::vector<Bone> bones{};

	auto append(std::span<Vertex const> vs, std::span<std::uint32_t const> is) -> Geometry&;

	auto append(Quad const& quad) -> Geometry&;
	auto append(Cube const& cube) -> Geometry&;
	auto append(Sphere const& sphere) -> Geometry&;

	template <typename ShapeT>
	static auto from(ShapeT const& shape) -> Geometry {
		auto ret = Geometry{};
		ret.append(shape);
		return ret;
	}
};

auto make_cone(float xz_diam, float y_height, std::uint32_t xz_points, glm::vec4 rgba = glm::vec4{1.0f}) -> Geometry;
auto make_cylinder(float xz_diam, float y_height, std::uint32_t xz_points, glm::vec4 rgba = glm::vec4{1.0f}) -> Geometry;
auto make_arrow(float stalk_diam, float stalk_height, std::uint32_t xz_points, glm::vec4 rgba = glm::vec4{1.0f}) -> Geometry;
auto make_manipulator(float stalk_diam, float stalk_height, std::uint32_t xz_points, glm::vec4 rgba = glm::vec4{1.0f}) -> Geometry;
} // namespace le::graphics
