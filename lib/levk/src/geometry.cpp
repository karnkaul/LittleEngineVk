#include <levk/core/is_positive.hpp>
#include <levk/geometry.hpp>
#include <levk/transform.hpp>
#include <algorithm>
#include <array>

namespace levk {
using namespace shape;

namespace {
struct Arc {
	Degrees start{};
	Degrees finish{};
};

struct Sector {
	glm::vec2 origin{};
	glm::vec4 rgba{};
	float radius{};
	Arc arc{};
	Degrees step{};
	UvRect uv{uv_rect_v};
};

auto append_sector(VertexArray& out, Sector const& sector) -> void {
	auto const& o = glm::vec3{sector.origin, 0.0f};
	auto const uvo = sector.uv.centre();
	auto const uvhe = 0.5f * sector.uv.size();
	auto const add_tri = [&](glm::vec2 const prev, glm::vec2 const curr) {
		// NOLINTNEXTLINE
		Vertex const vs[] = {
			{o, uvo, sector.rgba},
			{o + glm::vec3{sector.radius * prev, 0.0f}, uvo + prev * uvhe, sector.rgba},
			{o + glm::vec3{sector.radius * curr, 0.0f}, uvo + curr * uvhe, sector.rgba},
		};
		// NOLINTNEXTLINE
		std::uint32_t const is[] = {0, 1, 2};
		out.append(vs, is);
	};
	auto deg = sector.arc.start;
	auto rad = deg.to_radians();
	auto const get_dir = [](Radians const r) { return glm::vec2{-std::sin(r), std::cos(r)}; };
	auto prev = get_dir(rad);
	// NOLINTNEXTLINE
	for (deg = sector.arc.start + sector.step; deg < sector.arc.finish; deg.value += sector.step) {
		rad = deg.to_radians();
		auto const curr = get_dir(rad);
		add_tri(prev, curr);
		prev = curr;
	}
	add_tri(prev, get_dir(sector.arc.finish));
}

struct QuadWriter {
	VertexArray& out; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

	void append_quad(Quad const& quad) const {
		if (!is_positive(quad.size)) { return; }
		auto const vertices = make_vertices(quad);
		// NOLINTNEXTLINE
		std::uint32_t const indices[] = {
			0, 1, 2, 2, 3, 0,
		};
		out.append(vertices, indices);
	}

	void append_line_rect(LineRect const& rect) {
		if (!is_positive(rect.size)) { return; }
		auto const vertices = make_vertices(rect);
		// NOLINTNEXTLINE
		std::uint32_t const indices[] = {
			0, 1, 2, 3, 0,
		};
		out.append(vertices, indices);
	}

	[[nodiscard]] static auto make_vertices(Quad const& quad) -> std::array<Vertex, 4> {
		auto const half_size = 0.5f * quad.size;
		auto const& o = quad.origin;
		auto const rgba = colour::to_linear(quad.rgba);
		return std::array{
			Vertex{{o.x - half_size.x, o.y + half_size.y, 0.0f}, quad.uv.top_left(), rgba},
			Vertex{{o.x + half_size.x, o.y + half_size.y, 0.0f}, quad.uv.top_right(), rgba},
			Vertex{{o.x + half_size.x, o.y - half_size.y, 0.0f}, quad.uv.bottom_right(), rgba},
			Vertex{{o.x - half_size.x, o.y - half_size.y, 0.0f}, quad.uv.bottom_left(), rgba},
		};
	}
};

/*
	cells and their addresses:
	+----+--------+----+
	| 00 |   01   | 02 |
	+----+--------+----+
	|    |        |    |
	| 10 |   11   | 12 |
	|    |        |    |
	+----+--------+----+
	| 20 |   21   | 22 |
	+----+--------+----+
*/
struct QuadSlicer {
	NineQuad const& nine_quad; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

	std::array<std::array<Quad, 3>, 3> quads{};
	std::array<UvRect, 4> corner_uvs{};
	std::array<glm::vec2, 4> corner_sizes{};
	std::array<glm::vec2, 4> corner_origins{};

	QuadSlicer(NineQuad const& nine_quad) : nine_quad(nine_quad) {
		auto const top_uv = UvRect{.rb = nine_quad.slice.n_left_top};
		auto const bottom_uv = UvRect{.lt = nine_quad.slice.n_right_bottom, .rb = glm::vec2{1.0f}};
		corner_uvs = {
			top_uv,																			  // 00
			UvRect{.lt = {bottom_uv.lt.x, top_uv.lt.y}, .rb = {bottom_uv.rb.x, top_uv.rb.y}}, // 02
			UvRect{.lt = {top_uv.lt.x, bottom_uv.lt.y}, .rb = {top_uv.rb.x, bottom_uv.rb.y}}, // 20
			bottom_uv,																		  // 22
		};
		corner_sizes = {
			nine_quad.size.reference * nine_quad.slice.n_left_top,
			nine_quad.size.reference * glm::vec2{1.0f - nine_quad.slice.n_right_bottom.x, nine_quad.slice.n_left_top.y},
			nine_quad.size.reference * glm::vec2{nine_quad.slice.n_left_top.x, 1.0f - nine_quad.slice.n_right_bottom.y},
			nine_quad.size.reference * (1.0f - nine_quad.slice.n_right_bottom),
		};
		corner_origins = {
			nine_quad.origin + 0.5f * glm::vec2{-nine_quad.size.current.x + corner_sizes[0].x, nine_quad.size.current.y - corner_sizes[0].y},
			nine_quad.origin + 0.5f * (nine_quad.size.current - corner_sizes[1]),
			nine_quad.origin + 0.5f * (-nine_quad.size.current + corner_sizes[2]),
			nine_quad.origin + 0.5f * glm::vec2{nine_quad.size.current.x - corner_sizes[3].x, -nine_quad.size.current.y + corner_sizes[3].y},
		};

		for (auto& arr : quads) {
			for (auto& q : arr) { q.rgba = nine_quad.rgba; }
		}
	}

	void append_five_quad(VertexArray& out) {
		auto const x_horz = nine_quad.origin.x + 0.5f * (corner_sizes[0].x - corner_sizes[1].x);
		// 01
		quads[0][1].size = {
			nine_quad.size.current.x - corner_sizes[0].x - corner_sizes[1].x,
			corner_sizes[0].y,
		};
		quads[0][1].origin = {x_horz, corner_origins[0].y};
		quads[0][1].uv = UvRect{
			.lt = {corner_uvs[0].rb.x, corner_uvs[0].lt.y},
			.rb = {corner_uvs[1].lt.x, corner_uvs[1].rb.y},
		};
		out.append(quads[0][1]);

		// 21
		quads[2][1].size = {
			nine_quad.size.current.x - corner_sizes[2].x - corner_sizes[3].x,
			corner_sizes[2].y,
		};
		quads[2][1].origin = {x_horz, corner_origins[2].y};
		quads[2][1].uv = UvRect{
			.lt = {corner_uvs[2].rb.x, corner_uvs[2].lt.y},
			.rb = {corner_uvs[3].lt.x, corner_uvs[3].rb.y},
		};
		out.append(quads[2][1]);

		auto const y_vert = nine_quad.origin.y + 0.5f * (corner_sizes[2].y - corner_sizes[0].y);
		// 10
		quads[1][0].size = {
			corner_sizes[0].x,
			nine_quad.size.current.y - corner_sizes[0].y - corner_sizes[2].y,
		};
		quads[1][0].origin = {corner_origins[0].x, y_vert};
		quads[1][0].uv = UvRect{
			.lt = {corner_uvs[0].lt.x, corner_uvs[0].rb.y},
			.rb = {corner_uvs[2].rb.x, corner_uvs[2].lt.y},
		};
		out.append(quads[1][0]);

		// 12
		quads[1][2].size = {
			corner_sizes[1].x,
			nine_quad.size.current.y - corner_sizes[1].y - corner_sizes[3].y,
		};
		quads[1][2].origin = {corner_origins[1].x, y_vert};
		quads[1][2].uv = UvRect{
			.lt = {corner_uvs[1].lt.x, corner_uvs[1].rb.y},
			.rb = {corner_uvs[3].rb.x, corner_uvs[3].lt.y},
		};
		out.append(quads[1][2]);

		// 11
		quads[1][1].size = {quads[0][1].size.x, quads[1][0].size.y};
		quads[1][1].origin = {x_horz, y_vert};
		quads[1][1].uv = UvRect{.lt = corner_uvs[0].rb, .rb = corner_uvs[3].lt};
		out.append(quads[1][1]);
	}

	// corner cells: 00, 02, 20, 22 (these retain their size and UVs on size.current changing)
	void append_nine_quad(VertexArray& out) {
		// 00
		quads[0][0].size = corner_sizes[0];
		quads[0][0].origin = corner_origins[0];
		quads[0][0].uv = corner_uvs[0];
		out.append(quads[0][0]);

		// 02
		quads[0][2].size = corner_sizes[1];
		quads[0][2].origin = corner_origins[1];
		quads[0][2].uv = corner_uvs[1];
		out.append(quads[0][2]);

		// 20
		quads[2][0].size = corner_sizes[2];
		quads[2][0].origin = corner_origins[2];
		quads[2][0].uv = corner_uvs[2];
		out.append(quads[2][0]);

		// 22
		quads[2][2].size = corner_sizes[3];
		quads[2][2].origin = corner_origins[3];
		quads[2][2].uv = corner_uvs[3];
		out.append(quads[2][2]);

		append_five_quad(out);
	}

	void append_rounded_quad(VertexArray& out, int const corner_resolution) {
		auto const corner_arcs = std::array{
			Arc{.start = Degrees{0.0f}, .finish = Degrees{90.0f}},	  // 00
			Arc{.start = Degrees{270.0f}, .finish = Degrees{360.0f}}, // 02
			Arc{.start = Degrees{90.0f}, .finish = Degrees{180.0f}},  // 20
			Arc{.start = Degrees{180.0f}, .finish = Degrees{270.0f}}, // 22
		};
		// rounded corner offsets
		auto const ref_slice = NineSlice{
			.n_left_top = nine_quad.slice.n_left_top * nine_quad.size.reference,
			.n_right_bottom = (1.0f - nine_quad.slice.n_right_bottom) * nine_quad.size.reference,
		};
		auto const corner_offsets = std::array{
			0.5f * glm::vec2{ref_slice.n_left_top.x, -ref_slice.n_left_top.y},		   // 00
			0.5f * glm::vec2{-ref_slice.n_right_bottom.x, -ref_slice.n_left_top.y},	   // 02
			0.5f * glm::vec2{ref_slice.n_left_top.x, ref_slice.n_right_bottom.y},	   // 20
			0.5f * glm::vec2{-ref_slice.n_right_bottom.x, ref_slice.n_right_bottom.y}, // 22
		};

		auto const append_corner = [&](std::size_t index) {
			auto const arc = corner_arcs.at(index);
			auto const step = Degrees{(arc.finish - arc.start) / static_cast<float>(corner_resolution)};
			auto uv = corner_uvs.at(index);
			uv.lt *= 2.0f;
			uv.rb *= 2.0f;
			auto const sector = Sector{
				.origin = corner_origins.at(index) + corner_offsets.at(index),
				.rgba = colour::to_linear(nine_quad.rgba),
				.radius = std::min(corner_sizes.at(index).x, corner_sizes.at(index).y),
				.arc = arc,
				.step = step,
				.uv = uv,
			};
			append_sector(out, sector);
		};

		append_corner(0);
		append_corner(1);
		append_corner(2);
		append_corner(3);

		append_five_quad(out);
	}
};
} // namespace

auto VertexArray::append(std::span<Vertex const> vs, std::span<std::uint32_t const> is) -> VertexArray& {
	auto const i_offset = static_cast<std::uint32_t>(vertices.size());
	vertices.reserve(vertices.size() + vs.size());
	std::copy(vs.begin(), vs.end(), std::back_inserter(vertices));
	std::transform(is.begin(), is.end(), std::back_inserter(indices), [i_offset](std::uint32_t const i) { return i + i_offset; });
	return *this;
}

auto VertexArray::append(Quad const& quad) -> VertexArray& {
	QuadWriter{*this}.append_quad(quad);
	return *this;
}

auto VertexArray::append(Circle const& circle) -> VertexArray& {
	if (circle.diameter <= 0.0f) { return *this; }
	auto const finish = Degrees{360.0f};
	auto const resolution = circle.resolution;
	auto const sector = Sector{
		.origin = circle.origin,
		.rgba = colour::to_linear(circle.rgba),
		.radius = 0.5f * circle.diameter,
		.arc = {.finish = finish},
		.step = Degrees{finish / static_cast<float>(resolution)},
	};
	append_sector(*this, sector);
	return *this;
}

auto VertexArray::append(RoundedQuad const& rounded_quad) -> VertexArray& {
	if (rounded_quad.size.x <= 0.0f || rounded_quad.size.y <= 0.0f) { return *this; }
	if (rounded_quad.corner_radius <= 0.0f) { return append(static_cast<Quad const&>(rounded_quad)); }
	auto const corner_size = glm::vec2{rounded_quad.corner_radius};
	auto const n_corner_size = corner_size / rounded_quad.size;
	auto const nine_slice = NineSlice{.n_left_top = n_corner_size, .n_right_bottom = 1.0f - n_corner_size};
	auto const nine_quad = NineQuad{.size = rounded_quad.size, .slice = nine_slice, .rgba = rounded_quad.rgba, .origin = rounded_quad.origin};
	QuadSlicer{nine_quad}.append_rounded_quad(*this, rounded_quad.corner_resolution);
	return *this;
}

auto VertexArray::append(NineQuad const& nine_quad) -> VertexArray& {
	if (nine_quad.size.current.x <= 0.0f || nine_quad.size.current.y <= 0.0f) { return *this; }
	QuadSlicer{nine_quad}.append_nine_quad(*this);
	return *this;
}

auto VertexArray::append(LineRect const& rect) -> VertexArray& {
	QuadWriter{*this}.append_line_rect(rect);
	return *this;
}

auto VertexArray::append(Cube const& cube) -> VertexArray& {
	auto const half_size = 0.5f * cube.size;
	auto const& o = cube.origin;
	auto const rgba = colour::to_linear(cube.rgba);

	auto const z_verts = std::array{
		Vertex{{o.x - half_size.x, o.y + half_size.y, o.z + half_size.z}, uv_rect_v.top_left(), rgba, front_v},
		Vertex{{o.x + half_size.x, o.y + half_size.y, o.z + half_size.z}, uv_rect_v.top_right(), rgba, front_v},
		Vertex{{o.x + half_size.x, o.y - half_size.y, o.z + half_size.z}, uv_rect_v.bottom_right(), rgba, front_v},
		Vertex{{o.x - half_size.x, o.y - half_size.y, o.z + half_size.z}, uv_rect_v.bottom_left(), rgba, front_v},

		Vertex{{o.x - half_size.x, o.y + half_size.y, o.z - half_size.z}, uv_rect_v.top_left(), rgba, -front_v},
		Vertex{{o.x + half_size.x, o.y + half_size.y, o.z - half_size.z}, uv_rect_v.top_right(), rgba, -front_v},
		Vertex{{o.x + half_size.x, o.y - half_size.y, o.z - half_size.z}, uv_rect_v.bottom_right(), rgba, -front_v},
		Vertex{{o.x - half_size.x, o.y - half_size.y, o.z - half_size.z}, uv_rect_v.bottom_left(), rgba, -front_v},
	};

	auto const y_verts = std::array{
		Vertex{{o.x - half_size.x, o.y + half_size.y, o.z - half_size.z}, uv_rect_v.top_left(), rgba, up_v},
		Vertex{{o.x + half_size.x, o.y + half_size.y, o.z - half_size.z}, uv_rect_v.top_right(), rgba, up_v},
		Vertex{{o.x + half_size.x, o.y + half_size.y, o.z + half_size.z}, uv_rect_v.bottom_right(), rgba, up_v},
		Vertex{{o.x - half_size.x, o.y + half_size.y, o.z + half_size.z}, uv_rect_v.bottom_left(), rgba, up_v},

		Vertex{{o.x - half_size.x, o.y - half_size.y, o.z + half_size.z}, uv_rect_v.top_left(), rgba, -up_v},
		Vertex{{o.x + half_size.x, o.y - half_size.y, o.z + half_size.z}, uv_rect_v.top_right(), rgba, -up_v},
		Vertex{{o.x + half_size.x, o.y - half_size.y, o.z - half_size.z}, uv_rect_v.bottom_right(), rgba, -up_v},
		Vertex{{o.x - half_size.x, o.y - half_size.y, o.z - half_size.z}, uv_rect_v.bottom_left(), rgba, -up_v},
	};

	auto const x_verts = std::array{
		Vertex{{o.x - half_size.x, o.y + half_size.y, o.z - half_size.z}, uv_rect_v.top_left(), rgba, -right_v},
		Vertex{{o.x - half_size.x, o.y + half_size.y, o.z + half_size.z}, uv_rect_v.top_right(), rgba, -right_v},
		Vertex{{o.x - half_size.x, o.y - half_size.y, o.z + half_size.z}, uv_rect_v.bottom_right(), rgba, -right_v},
		Vertex{{o.x - half_size.x, o.y - half_size.y, o.z - half_size.z}, uv_rect_v.bottom_left(), rgba, -right_v},

		Vertex{{o.x + half_size.x, o.y + half_size.y, o.z + half_size.z}, uv_rect_v.top_left(), rgba, right_v},
		Vertex{{o.x + half_size.x, o.y + half_size.y, o.z - half_size.z}, uv_rect_v.top_right(), rgba, right_v},
		Vertex{{o.x + half_size.x, o.y - half_size.y, o.z - half_size.z}, uv_rect_v.bottom_right(), rgba, right_v},
		Vertex{{o.x + half_size.x, o.y - half_size.y, o.z + half_size.z}, uv_rect_v.bottom_left(), rgba, right_v},
	};

	auto const indices_x2 = std::array{
		0u, 1u, 2u, 2u, 3u, 0u, 4u, 5u, 6u, 6u, 7u, 4u,
	};

	vertices.reserve(vertices.size() + z_verts.size() + y_verts.size() + x_verts.size());
	indices.reserve(indices.size() + 3 * indices_x2.size());
	append(z_verts, indices_x2);
	append(y_verts, indices_x2);
	append(x_verts, indices_x2);

	return *this;
}

auto Geometry::get_triangle_count() const -> std::int64_t {
	auto const vertex_count = [&] {
		if (vertex_array.has_indices()) { return vertex_array.indices.size(); }
		return vertex_array.vertices.size();
	}();
	return compute_triangle_count(vertex_count, topology);
}
} // namespace levk
