#include <le/core/nvec3.hpp>
#include <le/core/radians.hpp>
#include <le/graphics/geometry.hpp>
#include <le/graphics/rect.hpp>
#include <algorithm>
#include <array>
#include <optional>

namespace le {
namespace graphics {
namespace {
struct Arc {
	Degrees start{};
	Degrees finish{};
};

struct Sector {
	glm::vec3 origin{};
	glm::vec4 rgba{};
	float radius{};
	Arc arc{};
	Degrees step{};
	UvRect uv{uv_rect_v};
};

struct Sliced {
	NineSlice const& nine_slice;
	int corner_resolution{};
};

auto append_sector(Geometry& out, Sector const& sector) -> void {
	auto const& o = sector.origin;
	auto const uvo = sector.uv.centre();
	auto const uvhe = 0.5f * sector.uv.extent();
	auto const add_tri = [&](glm::vec2 const prev, glm::vec2 const curr) {
		// NOLINTNEXTLINE
		Vertex const vs[] = {
			{o, sector.rgba, front_v, uvo},
			{o + glm::vec3{sector.radius * prev, 0.0f}, sector.rgba, front_v, uvo + prev * uvhe},
			{o + glm::vec3{sector.radius * curr, 0.0f}, sector.rgba, front_v, uvo + curr * uvhe},
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

auto append_sliced(Geometry& out, Sliced const& sliced) -> void {
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

		corner cells: 00, 02, 20, 22 (these retain their size and UVs on size.total changing)
	*/

	auto const& nine_slice = sliced.nine_slice;
	auto const half_size = 0.5f * nine_slice.size.total;
	auto const corner_uvs = std::array{
		nine_slice.top_uv,																											  // 00
		UvRect{.lt = {nine_slice.bottom_uv.lt.x, nine_slice.top_uv.lt.y}, .rb = {nine_slice.bottom_uv.rb.x, nine_slice.top_uv.rb.y}}, // 02
		UvRect{.lt = {nine_slice.top_uv.lt.x, nine_slice.bottom_uv.lt.y}, .rb = {nine_slice.top_uv.rb.x, nine_slice.bottom_uv.rb.y}}, // 20
		nine_slice.bottom_uv,																										  // 22
	};
	// origin offsets
	auto const cell_offsets = std::array{
		glm::vec2{-half_size.x + 0.5f * nine_slice.size.top.x, half_size.y - 0.5f * nine_slice.size.top.y},								 // 00
		glm::vec2{0.5f * (nine_slice.size.top.x - nine_slice.size.bottom.x), 0.5f * (nine_slice.size.bottom.y - nine_slice.size.top.y)}, // 11
		glm::vec2{half_size.x - 0.5f * nine_slice.size.bottom.x, 0.5f * (-nine_slice.size.total.y + nine_slice.size.bottom.y)},			 // 22
	};
	// rounded corner arcs
	auto const corner_arcs = std::array{
		Arc{.start = Degrees{0.0f}, .finish = Degrees{90.0f}},	  // 00
		Arc{.start = Degrees{270.0f}, .finish = Degrees{360.0f}}, // 02
		Arc{.start = Degrees{90.0f}, .finish = Degrees{180.0f}},  // 20
		Arc{.start = Degrees{180.0f}, .finish = Degrees{270.0f}}, // 22
	};
	// rounded corner offsets
	auto const corner_offsets = std::array{
		0.5f * glm::vec3{nine_slice.size.top.x, -nine_slice.size.top.y, 0.0f},		 // 00
		0.5f * glm::vec3{-nine_slice.size.bottom.x, -nine_slice.size.top.y, 0.0f},	 // 02
		0.5f * glm::vec3{nine_slice.size.top.x, nine_slice.size.bottom.y, 0.0f},	 // 20
		0.5f * glm::vec3{-nine_slice.size.bottom.x, nine_slice.size.bottom.y, 0.0f}, // 22
	};

	auto const rgba = Rgba::to_linear(nine_slice.rgba.to_vec4());
	auto const append_corner = [&](std::size_t index, glm::vec2 size, UvRect uv, glm::vec3 const& origin) {
		if (sliced.corner_resolution > 1) {
			auto const arc = corner_arcs.at(index);
			auto const step = Degrees{(arc.finish - arc.start) / static_cast<float>(sliced.corner_resolution)};
			uv.lt *= 2.0f;
			uv.rb *= 2.0f;
			auto const sector = Sector{
				.origin = origin + corner_offsets.at(index),
				.rgba = rgba,
				.radius = std::min(size.x, size.y),
				.arc = arc,
				.step = step,
				.uv = uv,
			};
			append_sector(out, sector);
		} else {
			out.append(Quad{.size = size, .uv = uv, .rgba = nine_slice.rgba, .origin = origin});
		}
	};

	auto const size_00 = nine_slice.size.top;
	auto const origin_00 = nine_slice.origin + glm::vec3{cell_offsets[0].x, cell_offsets[0].y, 0.0f};
	append_corner(0, size_00, corner_uvs[0], origin_00);

	auto const size_01 = glm::vec2{nine_slice.size.total.x - (nine_slice.size.top.x + nine_slice.size.bottom.x), nine_slice.size.top.y};
	auto const origin_01 = nine_slice.origin + glm::vec3{cell_offsets[1].x, cell_offsets[0].y, 0.0f};
	auto const uv_01 = UvRect{.lt = corner_uvs[0].top_right(), .rb = corner_uvs[1].bottom_left()};
	out.append(Quad{.size = size_01, .uv = uv_01, .rgba = nine_slice.rgba, .origin = origin_01});

	auto const size_02 = glm::vec2{nine_slice.size.bottom.x, nine_slice.size.top.y};
	auto const origin_02 = nine_slice.origin + glm::vec3{cell_offsets[2].x, cell_offsets[0].y, 0.0f};
	append_corner(1, size_02, corner_uvs[1], origin_02);

	auto const size_10 = glm::vec2{nine_slice.size.top.x, nine_slice.size.total.y - (nine_slice.size.top.y + nine_slice.size.bottom.y)};
	auto const origin_10 = nine_slice.origin + glm::vec3{cell_offsets[0].x, cell_offsets[1].y, 0.0f};
	auto const uv_10 = UvRect{.lt = corner_uvs[0].bottom_left(), .rb = corner_uvs[2].top_right()};
	out.append(Quad{.size = size_10, .uv = uv_10, .rgba = nine_slice.rgba, .origin = origin_10});

	auto const size_11 = nine_slice.size.total - (nine_slice.size.top + nine_slice.size.bottom);
	auto const origin_11 = nine_slice.origin + glm::vec3{cell_offsets[1].x, cell_offsets[1].y, 0.0f};
	auto const uv_11 = UvRect{.lt = corner_uvs[0].bottom_right(), .rb = corner_uvs[3].top_left()};
	out.append(Quad{.size = size_11, .uv = uv_11, .rgba = nine_slice.rgba, .origin = origin_11});

	auto const size_12 = glm::vec2{nine_slice.size.bottom.x, nine_slice.size.total.y - (nine_slice.size.top.y + nine_slice.size.bottom.y)};
	auto const origin_12 = nine_slice.origin + glm::vec3{cell_offsets[2].x, cell_offsets[1].y, 0.0f};
	auto const uv_12 = UvRect{.lt = corner_uvs[1].bottom_left(), .rb = corner_uvs[3].top_right()};
	out.append(Quad{.size = size_12, .uv = uv_12, .rgba = nine_slice.rgba, .origin = origin_12});

	auto const size_20 = glm::vec2{nine_slice.size.top.x, nine_slice.size.bottom.y};
	auto const origin_20 = nine_slice.origin + glm::vec3{cell_offsets[0].x, cell_offsets[2].y, 0.0f};
	append_corner(2, size_20, corner_uvs[2], origin_20);

	auto const size_21 = glm::vec2{nine_slice.size.total.x - (nine_slice.size.top.x + nine_slice.size.bottom.x), nine_slice.size.bottom.y};
	auto const origin_21 = nine_slice.origin + glm::vec3{cell_offsets[1].x, cell_offsets[2].y, 0.0f};
	auto const uv_21 = UvRect{.lt = corner_uvs[2].top_right(), .rb = corner_uvs[3].bottom_left()};
	out.append(Quad{.size = size_21, .uv = uv_21, .rgba = nine_slice.rgba, .origin = origin_21});

	auto const size_22 = nine_slice.size.bottom;
	auto const origin_22 = nine_slice.origin + glm::vec3{cell_offsets[2].x, cell_offsets[2].y, 0.0f};
	append_corner(3, size_22, corner_uvs[3], origin_22);
}
} // namespace

auto NineSlice::Size::rescaled(glm::vec2 const extent) const -> Size {
	assert(total.x > 0.0f && total.y > 0.0f);
	auto ret = *this;
	ret.top *= extent / total;
	ret.bottom *= extent / total;
	ret.total = extent;
	return ret;
}

auto Geometry::append(std::span<Vertex const> vs, std::span<std::uint32_t const> is) -> Geometry& {
	auto const i_offset = static_cast<std::uint32_t>(vertices.size());
	vertices.reserve(vertices.size() + vs.size());
	std::copy(vs.begin(), vs.end(), std::back_inserter(vertices));
	std::transform(is.begin(), is.end(), std::back_inserter(indices), [i_offset](std::uint32_t const i) { return i + i_offset; });
	return *this;
}

auto Geometry::append(Quad const& quad) -> Geometry& {
	if (quad.size.x <= 0.0f || quad.size.y <= 0.0f) { return *this; }
	auto const h = 0.5f * quad.size;
	auto const& o = quad.origin;
	auto const rgba = Rgba::to_linear(quad.rgba.to_vec4());
	// NOLINTNEXTLINE
	Vertex const vs[] = {
		{{o.x - h.x, o.y + h.y, 0.0f}, rgba, front_v, quad.uv.top_left()},
		{{o.x + h.x, o.y + h.y, 0.0f}, rgba, front_v, quad.uv.top_right()},
		{{o.x + h.x, o.y - h.y, 0.0f}, rgba, front_v, quad.uv.bottom_right()},
		{{o.x - h.x, o.y - h.y, 0.0f}, rgba, front_v, quad.uv.bottom_left()},
	};
	// NOLINTNEXTLINE
	std::uint32_t const is[] = {
		0, 1, 2, 2, 3, 0,
	};
	return append(vs, is);
}

auto Geometry::append(Circle const& circle) -> Geometry& {
	if (circle.diameter <= 0.0f) { return *this; }
	auto const finish = Degrees{360.0f};
	auto const resolution = circle.resolution;
	auto const sector = Sector{
		.origin = circle.origin,
		.rgba = Rgba::to_linear(circle.rgba.to_vec4()),
		.radius = 0.5f * circle.diameter,
		.arc = {.finish = finish},
		.step = Degrees{finish / static_cast<float>(resolution)},
	};
	append_sector(*this, sector);
	return *this;
}

auto Geometry::append(RoundedQuad const& rounded_quad) -> Geometry& {
	if (rounded_quad.size.x <= 0.0f || rounded_quad.size.y <= 0.0f) { return *this; }
	if (rounded_quad.corner_radius <= 0.0f) { return append(static_cast<Quad const&>(rounded_quad)); }
	auto const corner_size = glm::vec2{rounded_quad.corner_radius};
	auto const n_corner_size = corner_size / rounded_quad.size;
	auto const nine_slice = NineSlice{
		.size = NineSlice::Size{.total = rounded_quad.size, .top = corner_size, .bottom = corner_size},
		.top_uv = UvRect{.lt = rounded_quad.uv.lt, .rb = rounded_quad.uv.lt + n_corner_size},
		.bottom_uv = UvRect{.lt = rounded_quad.uv.rb - n_corner_size, .rb = rounded_quad.uv.rb},
		.rgba = rounded_quad.rgba,
		.origin = rounded_quad.origin,
	};
	auto sliced = Sliced{.nine_slice = nine_slice, .corner_resolution = rounded_quad.corner_resolution};
	append_sliced(*this, sliced);
	return *this;
}

auto Geometry::append(NineSlice const& nine_slice) -> Geometry& {
	if (nine_slice.size.total.x <= 0.0f || nine_slice.size.total.y <= 0.0f) { return *this; }
	auto const sliced = Sliced{.nine_slice = nine_slice};
	append_sliced(*this, sliced);
	return *this;
}

auto Geometry::append(Cube const& cube) -> Geometry& {
	if (cube.size.x <= 0.0f || cube.size.y <= 0.0f) { return *this; }
	auto const h = 0.5f * cube.size;
	auto const& o = cube.origin;
	auto const rgba = Rgba::to_linear(cube.rgba.to_vec4());
	// NOLINTNEXTLINE
	Vertex const vs[] = {
		// front
		{{o.x - h.x, o.y + h.y, o.z + h.z}, rgba, front_v, {0.0f, 0.0f}},
		{{o.x + h.x, o.y + h.y, o.z + h.z}, rgba, front_v, {1.0f, 0.0f}},
		{{o.x + h.x, o.y - h.y, o.z + h.z}, rgba, front_v, {1.0f, 1.0f}},
		{{o.x - h.x, o.y - h.y, o.z + h.z}, rgba, front_v, {0.0f, 1.0f}},

		// back
		{{o.x + h.x, o.y + h.y, o.z - h.z}, rgba, -front_v, {0.0f, 0.0f}},
		{{o.x - h.x, o.y + h.y, o.z - h.z}, rgba, -front_v, {1.0f, 0.0f}},
		{{o.x - h.x, o.y - h.y, o.z - h.z}, rgba, -front_v, {1.0f, 1.0f}},
		{{o.x + h.x, o.y - h.y, o.z - h.z}, rgba, -front_v, {0.0f, 1.0f}},

		// right
		{{o.x + h.x, o.y + h.y, o.z + h.z}, rgba, right_v, {0.0f, 0.0f}},
		{{o.x + h.x, o.y + h.y, o.z - h.z}, rgba, right_v, {1.0f, 0.0f}},
		{{o.x + h.x, o.y - h.y, o.z - h.z}, rgba, right_v, {1.0f, 1.0f}},
		{{o.x + h.x, o.y - h.y, o.z + h.z}, rgba, right_v, {0.0f, 1.0f}},

		// left
		{{o.x - h.x, o.y + h.y, o.z - h.z}, rgba, -right_v, {0.0f, 0.0f}},
		{{o.x - h.x, o.y + h.y, o.z + h.z}, rgba, -right_v, {1.0f, 0.0f}},
		{{o.x - h.x, o.y - h.y, o.z + h.z}, rgba, -right_v, {1.0f, 1.0f}},
		{{o.x - h.x, o.y - h.y, o.z - h.z}, rgba, -right_v, {0.0f, 1.0f}},

		// top
		{{o.x - h.x, o.y + h.y, o.z - h.z}, rgba, up_v, {0.0f, 0.0f}},
		{{o.x + h.x, o.y + h.y, o.z - h.z}, rgba, up_v, {1.0f, 0.0f}},
		{{o.x + h.x, o.y + h.y, o.z + h.z}, rgba, up_v, {1.0f, 1.0f}},
		{{o.x - h.x, o.y + h.y, o.z + h.z}, rgba, up_v, {0.0f, 1.0f}},

		// bottom
		{{o.x - h.x, o.y - h.y, o.z + h.z}, rgba, -up_v, {0.0f, 0.0f}},
		{{o.x + h.x, o.y - h.y, o.z + h.z}, rgba, -up_v, {1.0f, 0.0f}},
		{{o.x + h.x, o.y - h.y, o.z - h.z}, rgba, -up_v, {1.0f, 1.0f}},
		{{o.x - h.x, o.y - h.y, o.z - h.z}, rgba, -up_v, {0.0f, 1.0f}},
	};
	// clang-format off
	// NOLINTNEXTLINE
	std::uint32_t const is[] = {
		 0,  1,  2,  2,  3,  0,
		 4,  5,  6,  6,  7,  4,
		 8,  9, 10, 10, 11,  8,
		12, 13, 14, 14, 15, 12,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20,
	};
	// clang-format on

	return append(vs, is);
}

auto Geometry::append(Sphere const& sphere) -> Geometry& {
	if (sphere.diameter <= 0.0f) { return *this; }
	struct {
		std::vector<Vertex> vertices{};
		std::vector<std::uint32_t> indices{};
	} scratch{};

	// NOLINTNEXTLINE
	scratch.vertices.reserve(sphere.resolution * 4 * 6);
	// NOLINTNEXTLINE
	scratch.indices.reserve(sphere.resolution * 6 * 6);

	auto const bl = glm::vec3{-1.0f, -1.0f, 1.0f};
	auto points = std::vector<std::pair<glm::vec3, glm::vec2>>{};
	points.reserve(4 * sphere.resolution);
	auto const s = 2.0f / static_cast<float>(sphere.resolution);
	auto const duv = 1.0f / static_cast<float>(sphere.resolution);
	float u = 0.0f;
	float v = 0.0f;
	for (std::uint32_t row = 0; row < sphere.resolution; ++row) {
		v = static_cast<float>(row) * duv;
		for (std::uint32_t col = 0; col < sphere.resolution; ++col) {
			u = static_cast<float>(col) * duv;
			auto const o = s * glm::vec3{static_cast<float>(col), static_cast<float>(row), 0.0f};
			points.push_back({glm::vec3(bl + o), glm::vec2{u, 1.0f - v}});
			points.push_back({glm::vec3(bl + glm::vec3{s, 0.0f, 0.0f} + o), glm::vec2{u + duv, 1.0 - v}});
			points.push_back({glm::vec3(bl + glm::vec3{s, s, 0.0f} + o), glm::vec2{u + duv, 1.0f - v - duv}});
			points.push_back({glm::vec3(bl + glm::vec3{0.0f, s, 0.0f} + o), glm::vec2{u, 1.0f - v - duv}});
		}
	}

	auto const rgba = Rgba::to_linear(sphere.rgba.to_vec4());
	auto add_side = [&sphere, &scratch, rgba](std::vector<std::pair<glm::vec3, glm::vec2>>& out_points, nvec3 (*transform)(glm::vec3 const&)) {
		auto indices = std::array<std::uint32_t, 4>{};
		auto next_index = std::size_t{};
		auto update_indices = [&] {
			if (next_index >= 4) {
				std::copy(indices.begin(), indices.end(), std::back_inserter(scratch.indices));
				next_index = {};
			}
		};
		for (auto const& p : out_points) {
			update_indices();
			auto const pt = transform(p.first).value() * sphere.diameter * 0.5f;
			// NOLINTNEXTLINE
			indices[next_index++] = static_cast<std::uint32_t>(scratch.vertices.size());
			scratch.vertices.push_back({pt, rgba, pt, p.second});
		}
		update_indices();
	};
	add_side(points, [](glm::vec3 const& p) { return nvec3(p); });
	// NOLINTNEXTLINE
	add_side(points, [](glm::vec3 const& p) { return nvec3(glm::rotate(p, glm::radians(180.0f), up_v)); });
	// NOLINTNEXTLINE
	add_side(points, [](glm::vec3 const& p) { return nvec3(glm::rotate(p, glm::radians(90.0f), up_v)); });
	// NOLINTNEXTLINE
	add_side(points, [](glm::vec3 const& p) { return nvec3(glm::rotate(p, glm::radians(-90.0f), up_v)); });
	// NOLINTNEXTLINE
	add_side(points, [](glm::vec3 const& p) { return nvec3(glm::rotate(p, glm::radians(90.0f), right_v)); });
	// NOLINTNEXTLINE
	add_side(points, [](glm::vec3 const& p) { return nvec3(glm::rotate(p, glm::radians(-90.0f), right_v)); });

	for (auto& vertex : scratch.vertices) { vertex.position = sphere.origin + vertex.position; }

	append(scratch.vertices, scratch.indices);

	return *this;
}
} // namespace graphics

auto graphics::make_cone(float xz_diam, float y_height, std::uint32_t xz_points, glm::vec4 rgba) -> Geometry {
	if (xz_diam <= 0.0f || y_height <= 0.0f) { return {}; }
	auto ret = Geometry{};
	auto const r = 0.5f * xz_diam;
	auto const step = 360.0f / static_cast<float>(xz_points);
	auto add_tris = [&](glm::vec2 xz_a, glm::vec2 xz_b) {
		auto normal = glm::vec3{0.0f, 1.0f, 0.0f};
		auto const dy = 0.5f * y_height;
		// NOLINTNEXTLINE
		Vertex top_vs[] = {
			{{0.0f, dy, 0.0f}, rgba, {}, {}},
			{{r * xz_a.x, -dy, r * xz_a.y}, rgba, {}, xz_a},
			{{r * xz_b.x, -dy, r * xz_b.y}, rgba, {}, xz_b},
		};
		normal = glm::cross(top_vs[0].position - top_vs[2].position, top_vs[1].position - top_vs[2].position);
		for (auto& vert : top_vs) { vert.normal = normal; }
		// NOLINTNEXTLINE
		std::uint32_t const is[] = {0, 1, 2};
		ret.append(top_vs, is);
		normal = {0.0f, -1.0f, 0.0f};
		// NOLINTNEXTLINE
		Vertex const bottom_vs[] = {
			{{0.0f, -dy, 0.0f}, rgba, normal, {}},
			{{r * xz_a.x, -dy, r * xz_a.y}, rgba, normal, xz_a},
			{{r * xz_b.x, -dy, r * xz_b.y}, rgba, normal, xz_b},
		};
		ret.append(bottom_vs, is);
	};

	auto deg = 0.0f;
	auto prev = glm::vec2{1.0f, 0.0f};
	// NOLINTNEXTLINE
	for (deg = step; deg <= 360.0f; deg += step) {
		auto const rad = glm::radians(deg);
		auto const curr = glm::vec2{std::cos(rad), -std::sin(rad)};
		add_tris(prev, curr);
		prev = curr;
	}
	return ret;
}

auto graphics::make_cylinder(float xz_diam, float y_height, std::uint32_t xz_points, glm::vec4 rgba) -> Geometry {
	if (xz_diam <= 0.0f || y_height <= 0.0f) { return {}; }
	auto ret = Geometry{};
	auto const r = 0.5f * xz_diam;
	auto const step = 360.0f / static_cast<float>(xz_points);
	auto add_tris = [&](glm::vec2 xz_a, glm::vec2 xz_b) {
		auto normal = glm::vec3{0.0f, 1.0f, 0.0f};
		auto const dy = 0.5f * y_height;
		// top
		// NOLINTNEXTLINE
		Vertex const top_vs[] = {
			{{0.0f, dy, 0.0f}, rgba, normal, {}},
			{{r * xz_a.x, dy, r * xz_a.y}, rgba, normal, xz_a},
			{{r * xz_b.x, dy, r * xz_b.y}, rgba, normal, xz_b},
		};
		// bottom
		// NOLINTNEXTLINE
		Vertex const bottom_vs[] = {
			{{0.0f, -dy, 0.0f}, rgba, -normal, {}},
			{{r * xz_a.x, -dy, r * xz_a.y}, rgba, -normal, xz_a},
			{{r * xz_b.x, -dy, r * xz_b.y}, rgba, -normal, xz_b},
		};
		// NOLINTNEXTLINE
		std::uint32_t const is[] = {0, 1, 2};
		ret.append(top_vs, is);
		ret.append(bottom_vs, is);
		// centre
		// NOLINTNEXTLINE
		Vertex quad[] = {
			top_vs[1],
			top_vs[2],
			bottom_vs[2],
			bottom_vs[1],
		};
		normal = glm::cross(quad[1].position - quad[2].position, quad[0].position - quad[2].position);
		for (auto& vert : quad) { vert.normal = normal; }
		// NOLINTNEXTLINE
		std::uint32_t const quad_is[] = {0, 1, 2, 2, 3, 0};
		ret.append(quad, quad_is);
	};

	auto deg = 0.0f;
	auto prev = glm::vec2{1.0f, 0.0f};
	// NOLINTNEXTLINE
	for (deg = step; deg <= 360.0f; deg += step) {
		auto const rad = glm::radians(deg);
		auto const curr = glm::vec2{std::cos(rad), -std::sin(rad)};
		add_tris(prev, curr);
		prev = curr;
	}
	return ret;
}

auto graphics::make_arrow(float stalk_diam, float stalk_height, std::uint32_t xz_points, glm::vec4 rgba) -> Geometry {
	if (stalk_diam <= 0.0f || stalk_height <= 0.0f) { return {}; }
	auto const head_size = 2.0f * stalk_diam;
	auto ret = make_cone(head_size, head_size, xz_points, rgba);
	// NOLINTNEXTLINE
	for (auto& v : ret.vertices) { v.position.y += stalk_height + 0.5f * head_size; }
	auto stalk = make_cylinder(stalk_diam, stalk_height, xz_points, rgba);
	// NOLINTNEXTLINE
	for (auto& v : stalk.vertices) { v.position.y += 0.5f * stalk_height; }
	ret.append(stalk.vertices, stalk.indices);
	return ret;
}

auto graphics::make_manipulator(float stalk_diam, float stalk_height, std::uint32_t xz_points, glm::vec4 rgba) -> Geometry {
	if (stalk_diam <= 0.0f || stalk_height <= 0.0f) { return {}; }
	auto arrow = [&](std::optional<glm::vec3> const rotation) {
		auto ret = make_arrow(stalk_diam, stalk_height, xz_points, rgba);
		if (rotation) {
			for (auto& v : ret.vertices) {
				// NOLINTNEXTLINE
				v.position = glm::rotate(v.position, glm::radians(90.0f), *rotation);
				// NOLINTNEXTLINE
				v.normal = glm::normalize(glm::rotate(v.normal, glm::radians(90.0f), *rotation));
			}
		}
		return ret;
	};
	auto x = arrow(-front_v);
	auto const y = arrow({});
	auto const z = arrow(right_v);
	x.append(y.vertices, y.indices);
	x.append(z.vertices, z.indices);
	return x;
}
} // namespace le
