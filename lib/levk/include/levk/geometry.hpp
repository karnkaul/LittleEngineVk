#pragma once
#include <levk/colour.hpp>
#include <levk/core/radians.hpp>
#include <levk/rect.hpp>
#include <levk/vertex.hpp>
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <span>
#include <vector>

namespace levk {
namespace shape {
struct Quad;
struct Circle;
struct RoundedQuad;
struct NineQuad;
struct LineRect;
struct Cube;
} // namespace shape

/// \brief Collection of vertices and indices representing a graphics draw primitive.
struct VertexArray {
	std::vector<Vertex> vertices{};
	std::vector<std::uint32_t> indices{};

	auto append(std::span<Vertex const> vs, std::span<std::uint32_t const> is) -> VertexArray&;

	auto append(shape::Quad const& quad) -> VertexArray&;
	auto append(shape::Circle const& circle) -> VertexArray&;
	auto append(shape::RoundedQuad const& rounded_quad) -> VertexArray&;
	auto append(shape::NineQuad const& nine_quad) -> VertexArray&;
	auto append(shape::LineRect const& rect) -> VertexArray&;
	auto append(shape::Cube const& cube) -> VertexArray&;

	[[nodiscard]] auto is_empty() const -> bool { return vertices.empty(); }
	[[nodiscard]] auto has_indices() const -> bool { return !indices.empty(); }
	[[nodiscard]] auto size_bytes() const -> std::size_t { return std::span{vertices}.size_bytes() + std::span{indices}.size_bytes(); }
};

/// \brief Attributes for vertex skinning.
struct VertexSkin {
	std::vector<glm::uvec4> joint_indices{};
	std::vector<glm::vec4> weights{};

	[[nodiscard]] auto is_empty() const -> bool { return joint_indices.empty(); }
	[[nodiscard]] auto size_bytes() const -> std::size_t { return std::span{joint_indices}.size_bytes() + std::span{weights}.size_bytes(); }
};

[[nodiscard]] constexpr auto compute_triangle_count(std::size_t vertex_count, vk::PrimitiveTopology topology) -> std::int64_t;

/// \brief VertexArray and its Topology.
struct Geometry {
	static constexpr auto topology_v{vk::PrimitiveTopology::eTriangleList};

	VertexArray vertex_array{};
	VertexSkin vertex_skin{};
	vk::PrimitiveTopology topology{topology_v};

	template <typename ShapeT>
	static auto from(ShapeT const& shape, vk::PrimitiveTopology const toplogy = topology_v) -> Geometry {
		auto ret = Geometry{};
		ret.vertex_array.append(shape);
		ret.topology = toplogy;
		return ret;
	}

	[[nodiscard]] auto get_triangle_count() const -> std::int64_t;
};

namespace shape {
/// \brief Spec for an axis-aligned quad.
struct Quad {
	static constexpr auto size_v = glm::vec2{200.0f};

	glm::vec2 size{size_v};
	UvRect uv{uv_rect_v};
	RgbaU8 rgba{colour::white_v};
	glm::vec2 origin{};

	[[nodiscard]] auto to_geometry() const -> Geometry { return Geometry::from(*this); }

	auto operator==(Quad const&) const -> bool = default;
};

/// \brief Spec for a circle.
struct Circle {
	static constexpr int resolution_v{128};
	static constexpr auto diameter_v{200.0f};

	float diameter{diameter_v};
	int resolution{resolution_v};
	RgbaU8 rgba{colour::white_v};
	glm::vec2 origin{};

	[[nodiscard]] auto to_geometry() const -> Geometry { return Geometry::from(*this); }

	auto operator==(Circle const&) const -> bool = default;
};

/// \brief Spec for a quad with rounded corners.
struct RoundedQuad : Quad {
	float corner_radius{0.25f * size_v.x};
	int corner_resolution{8};

	[[nodiscard]] auto to_geometry() const -> Geometry { return Geometry::from(*this); }

	auto operator==(RoundedQuad const&) const -> bool = default;
};

/// \brief Spec for a 9-slice.
struct NineSlice {
	glm::vec2 n_left_top{0.25f};
	glm::vec2 n_right_bottom{0.75f};

	auto operator==(NineSlice const&) const -> bool = default;
};

/// \brief Spec for a 9-sliced axis-aligned quad.
struct NineQuad {
	struct Size {
		glm::vec2 reference{Quad::size_v};
		glm::vec2 current{Quad::size_v};

		constexpr Size(glm::vec2 const size = Quad::size_v) : Size(size, size) {}
		constexpr Size(glm::vec2 const reference, glm::vec2 const current) : reference(reference), current(current) {}

		auto operator==(Size const&) const -> bool = default;
	};

	Size size{};
	NineSlice slice{};
	RgbaU8 rgba{colour::white_v};
	glm::vec2 origin{};

	[[nodiscard]] auto to_geometry() const -> Geometry { return Geometry::from(*this); }

	auto operator==(NineQuad const&) const -> bool = default;
};

/// \brief Spec for a quad as a line strip.
struct LineRect : Quad {
	[[nodiscard]] auto to_geometry() const -> Geometry { return Geometry::from(*this, vk::PrimitiveTopology::eLineStrip); }
};

struct Cube {
	static constexpr auto size_v = glm::vec3{200.0f};

	glm::vec3 size{size_v};
	RgbaU8 rgba{colour::white_v};
	glm::vec3 origin{};

	[[nodiscard]] auto to_geometry() const -> Geometry { return Geometry::from(*this); }

	auto operator==(Cube const&) const -> bool = default;
};
} // namespace shape
} // namespace levk

// impl

constexpr auto levk::compute_triangle_count(std::size_t vertex_count, vk::PrimitiveTopology topology) -> std::int64_t {
	auto const ret = [&] {
		switch (topology) {
		case vk::PrimitiveTopology::eTriangleList: return vertex_count / 3;
		case vk::PrimitiveTopology::eTriangleStrip:
		case vk::PrimitiveTopology::eTriangleFan: return vertex_count - 2;
		default: return std::size_t{};
		}
	}();
	return static_cast<std::int64_t>(ret);
}
