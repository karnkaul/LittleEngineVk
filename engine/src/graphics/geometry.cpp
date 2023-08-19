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
auto append_sector(Geometry& out, glm::vec3 const& origin, glm::vec4 const& rgba, float radius, Degrees start, Degrees finish, Degrees step) -> void {
	auto const& o = origin;
	auto const add_tri = [&](glm::vec2 const prev, glm::vec2 const curr) {
		// NOLINTNEXTLINE
		Vertex const vs[] = {
			{o, rgba, front_v, glm::vec2{0.5f}},
			{o + glm::vec3{radius * prev, 0.0f}, rgba, front_v, prev},
			{o + glm::vec3{radius * curr, 0.0f}, rgba, front_v, curr},
		};
		// NOLINTNEXTLINE
		std::uint32_t const is[] = {0, 1, 2};
		out.append(vs, is);
	};
	auto deg = start;
	auto rad = deg.to_radians();
	auto prev = glm::vec2{std::cos(rad), -std::sin(rad)};
	// NOLINTNEXTLINE
	for (deg = start + step; deg <= finish; deg.value += step) {
		rad = deg.to_radians();
		auto const curr = glm::vec2{std::cos(rad), -std::sin(rad)};
		add_tri(prev, curr);
		prev = curr;
	}
}
} // namespace

auto NineSlice::rescaled(glm::vec2 const extent) const -> NineSlice {
	auto ret = *this;
	ret.top *= extent / size;
	ret.bottom *= extent / size;
	ret.size = extent;
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
	auto const h = 0.5f * quad.size;
	auto const& o = quad.origin;
	auto const rgba = quad.rgb.to_vec4();
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
	auto const rgba = circle.rgb.to_vec4();
	auto const radius = 0.5f * circle.diameter;
	auto const finish = Degrees{360.0f};
	auto const resolution = circle.resolution;
	auto const step = Degrees{finish / static_cast<float>(resolution)};
	append_sector(*this, circle.origin, rgba, radius, {}, finish, step);
	return *this;
}

auto Geometry::append(NineSlice const& nine_slice) -> Geometry& {
	auto const half_size = 0.5f * nine_slice.size;
	auto const corner_uvs = std::array{
		nine_slice.top_uv,
		UvRect{.lt = {nine_slice.bottom_uv.lt.x, nine_slice.top_uv.lt.y}, .rb = {nine_slice.bottom_uv.rb.x, nine_slice.top_uv.rb.y}},
		UvRect{.lt = {nine_slice.top_uv.lt.x, nine_slice.bottom_uv.lt.y}, .rb = {nine_slice.top_uv.rb.x, nine_slice.bottom_uv.rb.y}},
		nine_slice.bottom_uv,
	};
	auto const cell_offsets = std::array{
		glm::vec2{-half_size.x + 0.5f * nine_slice.top.x, half_size.y - 0.5f * nine_slice.top.y},
		glm::vec2{0.5f * (nine_slice.top.x - nine_slice.bottom.x), 0.5f * (nine_slice.bottom.y - nine_slice.top.y)},
		glm::vec2{half_size.x - 0.5f * nine_slice.bottom.x, 0.5f * (-nine_slice.size.y + nine_slice.bottom.y)},
	};

	auto const size_00 = nine_slice.top;
	auto const origin_00 = nine_slice.origin + glm::vec3{cell_offsets[0].x, cell_offsets[0].y, 0.0f};
	// TODO: rounded corners
	append(Quad{.size = size_00, .uv = corner_uvs[0], .origin = origin_00});

	auto const size_01 = glm::vec2{nine_slice.size.x - (nine_slice.top.x + nine_slice.bottom.x), nine_slice.top.y};
	auto const origin_01 = nine_slice.origin + glm::vec3{cell_offsets[1].x, cell_offsets[0].y, 0.0f};
	auto const uv_01 = UvRect{.lt = corner_uvs[0].top_right(), .rb = corner_uvs[1].bottom_left()};
	append(Quad{.size = size_01, .uv = uv_01, .origin = origin_01});

	auto const size_02 = glm::vec2{nine_slice.bottom.x, nine_slice.top.y};
	auto const origin_02 = nine_slice.origin + glm::vec3{cell_offsets[2].x, cell_offsets[0].y, 0.0f};
	append(Quad{.size = size_02, .uv = corner_uvs[1], .origin = origin_02});

	auto const size_10 = glm::vec2{nine_slice.top.x, nine_slice.size.y - (nine_slice.top.y + nine_slice.bottom.y)};
	auto const origin_10 = nine_slice.origin + glm::vec3{cell_offsets[0].x, cell_offsets[1].y, 0.0f};
	auto const uv_10 = UvRect{.lt = corner_uvs[0].bottom_left(), .rb = corner_uvs[2].top_right()};
	append(Quad{.size = size_10, .uv = uv_10, .origin = origin_10});

	auto const size_11 = nine_slice.size - (nine_slice.top + nine_slice.bottom);
	auto const origin_11 = nine_slice.origin + glm::vec3{cell_offsets[1].x, cell_offsets[1].y, 0.0f};
	auto const uv_11 = UvRect{.lt = corner_uvs[0].bottom_right(), .rb = corner_uvs[3].top_left()};
	append(Quad{.size = size_11, .uv = uv_11, .origin = origin_11});

	auto const size_12 = glm::vec2{nine_slice.bottom.x, nine_slice.size.y - (nine_slice.top.y + nine_slice.bottom.y)};
	auto const origin_12 = nine_slice.origin + glm::vec3{cell_offsets[2].x, cell_offsets[1].y, 0.0f};
	auto const uv_12 = UvRect{.lt = corner_uvs[1].bottom_left(), .rb = corner_uvs[3].top_right()};
	append(Quad{.size = size_12, .uv = uv_12, .origin = origin_12});

	auto const size_20 = glm::vec2{nine_slice.top.x, nine_slice.bottom.y};
	auto const origin_20 = nine_slice.origin + glm::vec3{cell_offsets[0].x, cell_offsets[2].y, 0.0f};
	append(Quad{.size = size_20, .uv = corner_uvs[2], .origin = origin_20});

	auto const size_21 = glm::vec2{nine_slice.size.x - (nine_slice.top.x + nine_slice.bottom.x), nine_slice.bottom.y};
	auto const origin_21 = nine_slice.origin + glm::vec3{cell_offsets[1].x, cell_offsets[2].y, 0.0f};
	auto const uv_21 = UvRect{.lt = corner_uvs[2].top_right(), .rb = corner_uvs[3].bottom_left()};
	append(Quad{.size = size_21, .uv = uv_21, .origin = origin_21});

	auto const size_22 = nine_slice.bottom;
	auto const origin_22 = nine_slice.origin + glm::vec3{cell_offsets[2].x, cell_offsets[2].y, 0.0f};
	append(Quad{.size = size_22, .uv = corner_uvs[3], .origin = origin_22});

	return *this;
}

auto Geometry::append(Cube const& cube) -> Geometry& {
	auto const h = 0.5f * cube.size;
	auto const& o = cube.origin;
	auto const rgba = cube.rgb.to_vec4();
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

	auto const rgba = sphere.rgb.to_vec4();
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
