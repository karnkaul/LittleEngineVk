#include <algorithm>
#include <iterator>
#include <core/ensure.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <graphics/geometry.hpp>

namespace le {
using v3 = glm::vec3;
using v2 = glm::vec2;

graphics::Geometry graphics::makeQuad(v2 size, v2 origin) {
	Geometry ret;
	f32 const x = size.x * 0.5f;
	f32 const y = size.y * 0.5f;
	// clang-format off
	ret.vertices = {
		{{-origin.x - x, -origin.y - y, 0.0f}, v3(1.0f), {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-origin.x + x, -origin.y - y, 0.0f}, v3(1.0f), {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-origin.x + x, -origin.y + y, 0.0f}, v3(1.0f), {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
		{{-origin.x - x, -origin.y + y, 0.0f}, v3(1.0f), {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
	};
	// clang-format on
	ret.indices = {0, 1, 2, 2, 3, 0};
	return ret;
}

graphics::Geometry graphics::makeCube(f32 side /* = 1.0f */) {
	Geometry ret;
	f32 const s = side * 0.5f;
	// clang-format off
	ret.vertices = {
		// back
		{{-s, -s,  s}, v3(1.0f), { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
		{{ s, -s,  s}, v3(1.0f), { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
		{{ s,  s,  s}, v3(1.0f), { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
		{{-s,  s,  s}, v3(1.0f), { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},
		// front
		{{-s, -s, -s}, v3(1.0f), { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
		{{ s, -s, -s}, v3(1.0f), { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
		{{ s,  s, -s}, v3(1.0f), { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
		{{-s,  s, -s}, v3(1.0f), { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
		// left
		{{-s,  s,  s}, v3(1.0f), {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
		{{-s,  s, -s}, v3(1.0f), {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
		{{-s, -s, -s}, v3(1.0f), {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
		{{-s, -s,  s}, v3(1.0f), {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
		// right
		{{ s,  s,  s}, v3(1.0f), { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
		{{ s,  s, -s}, v3(1.0f), { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
		{{ s, -s, -s}, v3(1.0f), { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
		{{ s, -s,  s}, v3(1.0f), { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
		// down
		{{-s, -s, -s}, v3(1.0f), { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
		{{ s, -s, -s}, v3(1.0f), { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
		{{ s, -s,  s}, v3(1.0f), { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
		{{-s, -s,  s}, v3(1.0f), { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
		// up
		{{-s,  s, -s}, v3(1.0f), { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},
		{{ s,  s, -s}, v3(1.0f), { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
		{{ s,  s,  s}, v3(1.0f), { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
		{{-s,  s,  s}, v3(1.0f), { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}
	};
	ret.indices = {
		 0,  1,  2,  2,  3,  0,
		 4,  5,  6,  6,  7,  4,
		 8,  9, 10, 10, 11,  8,
		12, 13, 14, 14, 15, 12,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20
	};
	// clang-format on
	return ret;
}

graphics::Geometry graphics::makeCircle(f32 diameter, u16 points) {
	ENSURE(points < 1000, "Max points is 1000");
	f32 const r = diameter * 0.5f;
	Geometry ret;
	f32 const arc = 360.0f / points;
	v3 const norm(0.0f, 0.0f, 1.0f);
	ret.reserve(1 + 1 + (u32)points, (1 + (u32)points) * 4);
	u32 const iCentre = ret.addVertex({{}, v3(1.0f), norm, {0.5f, 0.5f}});
	for (s32 i = 0; i <= points; ++i) {
		f32 const x1 = glm::cos(glm::radians(arc * (f32)i));
		f32 const y1 = glm::sin(glm::radians(arc * (f32)i));
		f32 const s1 = (x1 + 1.0f) * 0.5f;
		f32 const t1 = (y1 + 1.0f) * 0.5f;
		u32 const iv1 = ret.addVertex({{r * x1, r * y1, 0.0f}, v3(1.0f), norm, {s1, t1}});
		if (i > 0) {
			std::array const i = {iCentre, iv1 - 1, iv1};
			ret.addIndices(i);
		}
	}
	return ret;
}

graphics::Geometry graphics::makeCone(f32 diam, f32 height, u16 points) {
	ENSURE(points < 1000, "Max points is 1000");
	f32 const r = diam * 0.5f;
	Geometry verts;
	f32 const angle = 360.0f / points;
	v3 const nBase(0.0f, -1.0f, 0.0f);
	verts.reserve(1 + (u32)points * 5, (u32)points * 5 * 3);
	u32 const baseCentre = verts.addVertex({v3(0.0f), v3(1.0f), nBase, v2(0.5f)});
	for (s32 i = 0; i < points; ++i) {
		f32 const x0 = glm::cos(glm::radians(angle * (f32)i));
		f32 const z0 = glm::sin(glm::radians(angle * (f32)i));
		f32 const x1 = glm::cos(glm::radians(angle * (f32)(i + 1)));
		f32 const z1 = glm::sin(glm::radians(angle * (f32)(i + 1)));
		f32 const s0 = (x0 + 1.0f) * 0.5f;
		f32 const t0 = (z0 + 1.0f) * 0.5f;
		f32 const s1 = (x1 + 1.0f) * 0.5f;
		f32 const t1 = (z1 + 1.0f) * 0.5f;
		v3 const v0(r * x0, 0.0f, r * z0);
		v3 const v1(r * x1, 0.0f, r * z1);
		// Base circle arc
		u32 const i0 = verts.addVertex({v0, v3(1.0f), nBase, {s0, t0}});
		u32 const i1 = verts.addVertex({v1, v3(1.0f), nBase, {s1, t1}});
		std::array const arr0 = {baseCentre, i1, i0};
		verts.addIndices(arr0); // reverse winding; bottom face is "front"
		// Face
		v3 const v2(0.0f, height, 0.0f);
		auto const nFace = glm::normalize(glm::cross(v2 - v0, v1 - v0));
		u32 const i2 = verts.addVertex({v0, v3(1.0f), nFace, {s0, 1.0f}});
		u32 const i3 = verts.addVertex({v1, v3(1.0f), nFace, {s1, 1.0f}});
		u32 const i4 = verts.addVertex({v2, v3(1.0f), nFace, {0.5f, 0.5f}});
		std::array const arr1 = {i2, i3, i4};
		verts.addIndices(arr1);
	}
	return verts;
}

graphics::Geometry graphics::makeCubedSphere(f32 diam, u8 quadsPerSide) {
	ENSURE(quadsPerSide < 30, "Max quads per side is 30");
	Geometry ret;
	u32 qCount = (u32)(quadsPerSide * quadsPerSide);
	ret.reserve(qCount * 4 * 6, qCount * 6 * 6);
	v3 const bl(-1.0f, -1.0f, 1.0f);
	std::vector<std::pair<v3, v2>> points;
	points.reserve(4 * qCount);
	f32 const s = 2.0f / quadsPerSide;
	f32 const duv = 1.0f / quadsPerSide;
	f32 u = 0.0f;
	f32 v = 0.0f;
	for (s32 row = 0; row < quadsPerSide; ++row) {
		v = (f32)row * duv;
		for (s32 col = 0; col < quadsPerSide; ++col) {
			u = (f32)col * duv;
			v3 const o = s * v3((f32)col, (f32)row, 0.0f);
			points.push_back(std::make_pair(v3(bl + o), v2(u, 1.0f - v)));
			points.push_back(std::make_pair(v3(bl + v3(s, 0.0f, 0.0f) + o), v2(u + duv, 1.0 - v)));
			points.push_back(std::make_pair(v3(bl + v3(s, s, 0.0f) + o), v2(u + duv, 1.0f - v - duv)));
			points.push_back(std::make_pair(v3(bl + v3(0.0f, s, 0.0f) + o), v2(u, 1.0f - v - duv)));
		}
	}
	auto addSide = [](std::vector<std::pair<v3, v2>>& out_points, graphics::Geometry& out_ret, f32 diam, v3 (*transform)(v3 const&)) {
		s32 idx = 0;
		std::vector<u32> iV;
		iV.reserve(4);
		for (auto const& p : out_points) {
			if (iV.size() == 4) {
				std::array const arr = {iV[0], iV[1], iV[2], iV[2], iV[3], iV[0]};
				out_ret.addIndices(arr);
				iV.clear();
			}
			++idx;
			v3 const pt = transform(p.first) * diam * 0.5f;
			iV.push_back(out_ret.addVertex({pt, v3(1.0f), pt, p.second}));
		}
		if (iV.size() == 4) {
			std::array const arr = {iV[0], iV[1], iV[2], iV[2], iV[3], iV[0]};
			out_ret.addIndices(arr);
		}
	};
	addSide(points, ret, diam, [](v3 const& p) -> v3 { return glm::normalize(p); });
	addSide(points, ret, diam, [](v3 const& p) -> v3 { return glm::normalize(glm::rotate(p, glm::radians(180.0f), up)); });
	addSide(points, ret, diam, [](v3 const& p) -> v3 { return glm::normalize(glm::rotate(p, glm::radians(90.0f), up)); });
	addSide(points, ret, diam, [](v3 const& p) -> v3 { return glm::normalize(glm::rotate(p, glm::radians(-90.0f), up)); });
	addSide(points, ret, diam, [](v3 const& p) -> v3 { return glm::normalize(glm::rotate(p, glm::radians(90.0f), right)); });
	addSide(points, ret, diam, [](v3 const& p) -> v3 { return glm::normalize(glm::rotate(p, glm::radians(-90.0f), right)); });
	return ret;
}
} // namespace le
