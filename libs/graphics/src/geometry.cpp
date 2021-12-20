#include <core/utils/error.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <graphics/geometry.hpp>
#include <algorithm>
#include <iterator>

namespace le {
using v3 = glm::vec3;
using v2 = glm::vec2;

void graphics::IndexStitcher::add(u32 index) {
	indices.push_back(index);
	if (index >= pin && index - pin >= stride) {
		indices.push_back(index);
		pin = index;
	}
}

void graphics::IndexStitcher::reset(u32 index) {
	indices.push_back(start);
	start = pin = index;
}

graphics::Geometry graphics::makeQuad(glm::vec2 size, GeomInfo const& info, UVQuad const& uv, Topology topo) {
	Geometry ret;
	f32 const x = size.x * 0.5f;
	f32 const y = size.y * 0.5f;
	auto const& o = info.origin;
	auto const& c = info.colour;
	// clang-format off
	ret.vertices = {
		{{o.x - x, o.y - y, o.z}, c, {0.0f, 0.0f, 1.0f}, {uv.topLeft.x, uv.bottomRight.y}},
		{{o.x + x, o.y - y, o.z}, c, {0.0f, 0.0f, 1.0f}, uv.bottomRight},
		{{o.x + x, o.y + y, o.z}, c, {0.0f, 0.0f, 1.0f}, {uv.bottomRight.x, uv.topLeft.y}},
		{{o.x - x, o.y + y, o.z}, c, {0.0f, 0.0f, 1.0f}, uv.topLeft}
	};
	// clang-format on
	ret.autoIndex(topo, 4);
	return ret;
}

graphics::Geometry graphics::makeCube(f32 side /* = 1.0f */, GeomInfo const& info, Topology topo) {
	Geometry ret;
	f32 const s = side * 0.5f;
	auto const& o = info.origin;
	auto const& c = info.colour;
	// clang-format off
	ret.vertices = {
		// back
		{{o.x + -s, o.y + -s, o.z +  s}, c, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
		{{o.x +  s, o.y + -s, o.z +  s}, c, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
		{{o.x +  s, o.y +  s, o.z +  s}, c, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
		{{o.x + -s, o.y +  s, o.z +  s}, c, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},
		// front
		{{o.x + -s, o.y + -s, o.z + -s}, c, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
		{{o.x +  s, o.y + -s, o.z + -s}, c, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
		{{o.x +  s, o.y +  s, o.z + -s}, c, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
		{{o.x + -s, o.y +  s, o.z + -s}, c, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
		// left
		{{o.x + -s, o.y +  s, o.z +  s}, c, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
		{{o.x + -s, o.y +  s, o.z + -s}, c, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
		{{o.x + -s, o.y + -s, o.z + -s}, c, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
		{{o.x + -s, o.y + -s, o.z +  s}, c, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
		// right
		{{o.x +  s, o.y +  s, o.z +  s}, c, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
		{{o.x +  s, o.y +  s, o.z + -s}, c, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
		{{o.x +  s, o.y + -s, o.z + -s}, c, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
		{{o.x +  s, o.y + -s, o.z +  s}, c, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
		// down
		{{o.x + -s, o.y + -s, o.z + -s}, c, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
		{{o.x +  s, o.y + -s, o.z + -s}, c, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
		{{o.x +  s, o.y + -s, o.z +  s}, c, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
		{{o.x + -s, o.y + -s, o.z +  s}, c, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
		// up
		{{o.x + -s, o.y +  s, o.z + -s}, c, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},
		{{o.x +  s, o.y +  s, o.z + -s}, c, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
		{{o.x +  s, o.y +  s, o.z +  s}, c, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
		{{o.x + -s, o.y +  s, o.z +  s}, c, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}
	};
	// clang-format on
	ret.autoIndex(topo, 4);
	return ret;
}

graphics::Geometry graphics::makeSector(glm::vec2 radExtent, f32 diameter, u16 points, GeomInfo const& info) {
	Geometry ret;
	ENSURE(points < 1000, "Max points is 1000");
	f32 const subArc = (radExtent.y - radExtent.x) / points;
	f32 const r = diameter * 0.5f;
	v3 const norm(0.0f, 0.0f, 1.0f);
	v3 const& o = info.origin;
	ret.reserve(1 + 1 + (u32)points, (1 + (u32)points) * 4);
	v2 uv = v2(0.5f, 0.5f);
	u32 const iCentre = ret.addVertex({o, info.colour, norm, uv});
	for (u16 i = 0; i <= points; ++i) {
		f32 const x1 = glm::cos(subArc * (f32)i + radExtent.x);
		f32 const y1 = glm::sin(subArc * (f32)i + radExtent.x);
		f32 const s = (x1 + 1.0f) * 0.5f;
		f32 const t = (y1 + 1.0f) * 0.5f;
		uv = v2(s, 1.0f - t);
		u32 const iv1 = ret.addVertex({o + v3{r * x1, r * y1, 0.0f}, info.colour, norm, uv});
		if (i > 0) {
			std::array const i = {iCentre, iv1 - 1, iv1};
			ret.addIndices(i);
		}
	}
	return ret;
}

graphics::Geometry graphics::makeCircle(f32 diameter, u16 points, GeomInfo const& info) {
	return makeSector({0.0f, glm::pi<f32>() * 2.0f}, diameter, points, info);
}

graphics::Geometry graphics::makeCone(f32 diam, f32 height, u16 points, GeomInfo const& info) {
	Geometry ret;
	ENSURE(points < 1000, "Max points is 1000");
	f32 const r = diam * 0.5f;
	f32 const angle = 360.0f / points;
	v3 const nBase(0.0f, -1.0f, 0.0f);
	ret.reserve(1 + (u32)points * 5, (u32)points * 5 * 3);
	auto const& o = info.origin;
	u32 const baseCentre = ret.addVertex({v3(0.0f), info.colour, nBase, v2(0.5f)});
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
		u32 const i0 = ret.addVertex({o + v0, info.colour, nBase, {s0, t0}});
		u32 const i1 = ret.addVertex({o + v1, info.colour, nBase, {s1, t1}});
		std::array const arr0 = {baseCentre, i1, i0};
		ret.addIndices(arr0); // reverse winding; bottom face is "front"
		// Face
		v3 const v2(0.0f, height, 0.0f);
		auto const nFace = nvec3(glm::cross(v2 - v0, v1 - v0));
		u32 const i2 = ret.addVertex({o + v0, info.colour, nFace, {s0, 1.0f}});
		u32 const i3 = ret.addVertex({o + v1, info.colour, nFace, {s1, 1.0f}});
		u32 const i4 = ret.addVertex({o + v2, info.colour, nFace, {0.5f, 0.5f}});
		std::array const arr1 = {i2, i3, i4};
		ret.addIndices(arr1);
	}
	return ret;
}

graphics::Geometry graphics::makeCubedSphere(f32 diam, u8 quadsPerSide, GeomInfo const& info) {
	Geometry ret;
	ENSURE(quadsPerSide < 30, "Max quads per side is 30");
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
	auto addSide = [c = info.colour](std::vector<std::pair<v3, v2>>& out_points, graphics::Geometry& out_ret, f32 diam, nvec3 (*transform)(v3 const&)) {
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
			iV.push_back(out_ret.addVertex({pt, c, pt, p.second}));
		}
		if (iV.size() == 4) {
			std::array const arr = {iV[0], iV[1], iV[2], iV[2], iV[3], iV[0]};
			out_ret.addIndices(arr);
		}
	};
	addSide(points, ret, diam, [](v3 const& p) { return nvec3(p); });
	addSide(points, ret, diam, [](v3 const& p) { return nvec3(glm::rotate(p, glm::radians(180.0f), up)); });
	addSide(points, ret, diam, [](v3 const& p) { return nvec3(glm::rotate(p, glm::radians(90.0f), up)); });
	addSide(points, ret, diam, [](v3 const& p) { return nvec3(glm::rotate(p, glm::radians(-90.0f), up)); });
	addSide(points, ret, diam, [](v3 const& p) { return nvec3(glm::rotate(p, glm::radians(90.0f), right)); });
	addSide(points, ret, diam, [](v3 const& p) { return nvec3(glm::rotate(p, glm::radians(-90.0f), right)); });
	return ret;
}

graphics::Geometry graphics::makeRoundedQuad(glm::vec2 size, f32 radius, u16 points, GeomInfo const& info) {
	if (radius <= 0.0f) { return makeQuad(size, info); }
	Geometry ret;
	f32 const r = radius;
	v3 const& o = info.origin;
	auto const& c = info.colour;
	v2 const q = size - v2(radius * 2.0f, radius * 2.0f);
	f32 const hr = radius * 0.5f;
	auto left = makeQuad({r, q.y}, {{o.x - q.x * 0.5f - hr, o.y, o.z}, c}, {{0.0f, r / size.y}, {r / size.x, 1.0f - r / size.y}});
	auto right = makeQuad({r, q.y}, {{o.x + q.x * 0.5f + hr, o.y, o.z}, c}, {{1.0f - r / size.x, r / size.y}, {1.0f, 1.0f - r / size.y}});
	auto centre = makeQuad({q.x, size.y}, {o, c}, {{r / size.x, 0.0f}, {1.0f - r / size.x, 1.0f}});
	v2 const hs = q * 0.5f;
	f32 const hpi = glm::pi<f32>() * 0.5f;
	auto sectorUVs = [r](std::vector<Vertex>& out) {
		v2 const offset = out[0].texCoord - 0.5f; // transform to [-0.5, 0.5]
		for (std::size_t i = 1; i < out.size(); ++i) {
			v2 const cv = out[i].texCoord - 0.5f; // transform to [-0.5, 0.5]
			// transform cv to contribution (fake projection), multiply by coeff, move by offset
			v2 const st = offset + r * (cv * 2.0f);
			out[i].texCoord = st + 0.5f; // transform back to [0, 1]
		}
	};
	auto topRight = makeSector({0.0f, hpi}, r * 2.0f, points / 4, {o + v3(hs, 0.0f), c});
	topRight.vertices[0].texCoord = {1.0f - r / size.x, r / size.y};
	sectorUVs(topRight.vertices);
	auto topLeft = makeSector({hpi, 2.0f * hpi}, r * 2.0f, points / 4, {o + v3{-hs.x, hs.y, 0.0f}, c});
	topLeft.vertices[0].texCoord = {r / size.x, r / size.y};
	sectorUVs(topLeft.vertices);
	auto bottomLeft = makeSector({2.0f * hpi, 3.0f * hpi}, r * 2.0f, points / 4, {o - v3(hs, 0.0f), c});
	bottomLeft.vertices[0].texCoord = {r / size.x, 1.0f - r / size.y};
	sectorUVs(bottomLeft.vertices);
	auto bottomRight = makeSector({3.0f * hpi, 4.0f * hpi}, r * 2.0f, points / 4, {o + v3{hs.x, -hs.y, 0.0f}, c});
	bottomRight.vertices[0].texCoord = {1.0f - r / size.x, 1.0f - r / size.y};
	sectorUVs(bottomRight.vertices);
	ret.append(left, right, topRight, topLeft, bottomLeft, bottomRight, centre);
	return ret;
}
} // namespace le
