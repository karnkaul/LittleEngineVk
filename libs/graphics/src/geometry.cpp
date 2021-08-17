#include <algorithm>
#include <iterator>
#include <core/ensure.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <graphics/geometry.hpp>

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

graphics::Geometry graphics::makeQuad(v2 size, v2 origin, v3 colour, Topology topo) {
	Geometry ret;
	f32 const x = size.x * 0.5f;
	f32 const y = size.y * 0.5f;
	// clang-format off
	ret.vertices = {
		{{origin.x - x, origin.y - y, 0.0f}, colour, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{origin.x + x, origin.y - y, 0.0f}, colour, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{origin.x + x, origin.y + y, 0.0f}, colour, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
		{{origin.x - x, origin.y + y, 0.0f}, colour, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
	};
	// clang-format on
	ret.autoIndex(topo, 4);
	return ret;
}

graphics::Geometry graphics::makeCube(f32 side /* = 1.0f */, v3 o, v3 colour, Topology topo) {
	Geometry ret;
	f32 const s = side * 0.5f;
	// clang-format off
	ret.vertices = {
		// back
		{{o.x + -s, o.y + -s, o.z +  s}, colour, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
		{{o.x +  s, o.y + -s, o.z +  s}, colour, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
		{{o.x +  s, o.y +  s, o.z +  s}, colour, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
		{{o.x + -s, o.y +  s, o.z +  s}, colour, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},
		// front
		{{o.x + -s, o.y + -s, o.z + -s}, colour, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
		{{o.x +  s, o.y + -s, o.z + -s}, colour, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
		{{o.x +  s, o.y +  s, o.z + -s}, colour, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
		{{o.x + -s, o.y +  s, o.z + -s}, colour, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
		// left
		{{o.x + -s, o.y +  s, o.z +  s}, colour, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
		{{o.x + -s, o.y +  s, o.z + -s}, colour, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
		{{o.x + -s, o.y + -s, o.z + -s}, colour, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
		{{o.x + -s, o.y + -s, o.z +  s}, colour, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
		// right
		{{o.x +  s, o.y +  s, o.z +  s}, colour, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
		{{o.x +  s, o.y +  s, o.z + -s}, colour, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
		{{o.x +  s, o.y + -s, o.z + -s}, colour, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
		{{o.x +  s, o.y + -s, o.z +  s}, colour, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
		// down
		{{o.x + -s, o.y + -s, o.z + -s}, colour, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
		{{o.x +  s, o.y + -s, o.z + -s}, colour, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
		{{o.x +  s, o.y + -s, o.z +  s}, colour, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
		{{o.x + -s, o.y + -s, o.z +  s}, colour, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
		// up
		{{o.x + -s, o.y +  s, o.z + -s}, colour, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},
		{{o.x +  s, o.y +  s, o.z + -s}, colour, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
		{{o.x +  s, o.y +  s, o.z +  s}, colour, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
		{{o.x + -s, o.y +  s, o.z +  s}, colour, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}
	};
	// clang-format on
	ret.autoIndex(topo, 4);
	return ret;
}

graphics::Geometry graphics::makeSector(f32 radBegin, f32 radEnd, f32 diameter, u16 points, glm::vec2 origin, glm::vec3 colour) {
	ensure(points < 1000, "Max points is 1000");
	Geometry ret;
	f32 const subArc = (radEnd - radBegin) / points;
	f32 const r = diameter * 0.5f;
	v3 const norm(0.0f, 0.0f, 1.0f);
	v3 const o = {origin, 0.0f};
	ret.reserve(1 + 1 + (u32)points, (1 + (u32)points) * 4);
	v2 uv = v2(0.5f, 0.5f);
	u32 const iCentre = ret.addVertex({o, colour, norm, uv});
	for (u16 i = 0; i <= points; ++i) {
		f32 const x1 = glm::cos(subArc * (f32)i + radBegin);
		f32 const y1 = glm::sin(subArc * (f32)i + radBegin);
		f32 const s = (x1 + 1.0f) * 0.5f;
		f32 const t = (y1 + 1.0f) * 0.5f;
		uv = v2(s, 1.0f - t);
		u32 const iv1 = ret.addVertex({o + v3{r * x1, r * y1, 0.0f}, colour, norm, uv});
		if (i > 0) {
			std::array const i = {iCentre, iv1 - 1, iv1};
			ret.addIndices(i);
		}
	}
	return ret;
}

graphics::Geometry graphics::makeCircle(f32 diameter, u16 points, glm::vec2 origin, glm::vec3 colour) {
	return makeSector(0.0f, glm::pi<f32>() * 2.0f, diameter, points, origin, colour);
}

graphics::Geometry graphics::makeCone(f32 diam, f32 height, u16 points, glm::vec3 colour) {
	ensure(points < 1000, "Max points is 1000");
	f32 const r = diam * 0.5f;
	Geometry verts;
	f32 const angle = 360.0f / points;
	v3 const nBase(0.0f, -1.0f, 0.0f);
	verts.reserve(1 + (u32)points * 5, (u32)points * 5 * 3);
	u32 const baseCentre = verts.addVertex({v3(0.0f), colour, nBase, v2(0.5f)});
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
		u32 const i0 = verts.addVertex({v0, colour, nBase, {s0, t0}});
		u32 const i1 = verts.addVertex({v1, colour, nBase, {s1, t1}});
		std::array const arr0 = {baseCentre, i1, i0};
		verts.addIndices(arr0); // reverse winding; bottom face is "front"
		// Face
		v3 const v2(0.0f, height, 0.0f);
		auto const nFace = glm::normalize(glm::cross(v2 - v0, v1 - v0));
		u32 const i2 = verts.addVertex({v0, colour, nFace, {s0, 1.0f}});
		u32 const i3 = verts.addVertex({v1, colour, nFace, {s1, 1.0f}});
		u32 const i4 = verts.addVertex({v2, colour, nFace, {0.5f, 0.5f}});
		std::array const arr1 = {i2, i3, i4};
		verts.addIndices(arr1);
	}
	return verts;
}

graphics::Geometry graphics::makeCubedSphere(f32 diam, u8 quadsPerSide, glm::vec3 colour) {
	ensure(quadsPerSide < 30, "Max quads per side is 30");
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
	auto addSide = [colour](std::vector<std::pair<v3, v2>>& out_points, graphics::Geometry& out_ret, f32 diam, v3 (*transform)(v3 const&)) {
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
			iV.push_back(out_ret.addVertex({pt, colour, pt, p.second}));
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

graphics::Geometry graphics::makeRoundedQuad(glm::vec2 size, f32 radius, u16 points, glm::vec2 origin, glm::vec3 colour) {
	if (radius <= 0.0f) { return makeQuad(size, origin, colour); }
	v2 const csize = size - v2(radius * 2.0f, radius * 2.0f);
	f32 const halfRadius = radius * 0.5f;
	auto quadUVs = [](graphics::Geometry & out, v2 bl, v2 tr) {
		out.vertices[0].texCoord = bl;
		out.vertices[1].texCoord = {tr.x, bl.y};
		out.vertices[2].texCoord = tr;
		out.vertices[3].texCoord = {bl.x, tr.y};
	};
	auto left = makeQuad({radius, csize.y}, {origin.x - csize.x * 0.5f - halfRadius, origin.y}, colour);
	quadUVs(left, {0.0f, 1.0f - radius / size.y}, {radius / size.x, radius / size.y});
	auto right = makeQuad({radius, csize.y}, {origin.x + csize.x * 0.5f + halfRadius, origin.y}, colour);
	quadUVs(right, {1.0f - radius / size.x, 1.0f - radius / size.y}, {1.0f, radius / size.y});
	auto centre = makeQuad({csize.x, size.y}, origin, colour);
	quadUVs(centre, {radius / size.x, 1.0f}, {1.0f - radius / size.x, 0.0f});
	v2 const halfSize = csize * 0.5f;
	f32 const hpi = glm::pi<f32>() * 0.5f;
	auto sectorUVs = [radius](std::vector<Vertex>& out) {
		v2 const offset = out[0].texCoord - 0.5f; // transform to [-0.5, 0.5]
		for (std::size_t i = 1; i < out.size(); ++i) {
			v2 const cv = out[i].texCoord - 0.5f; // transform to [-0.5, 0.5]
			// transform cv to contribution (fake projection), multiply by coeff, move by offset
			v2 const st = offset + radius * (cv * 2.0f);
			out[i].texCoord = st + 0.5f; // transform back to [0, 1]
		}
	};
	auto topRight = makeSector(0.0f, hpi, radius * 2.0f, points / 4, origin + halfSize, colour);
	topRight.vertices[0].texCoord = {1.0f - radius / size.x, radius / size.y};
	sectorUVs(topRight.vertices);
	auto topLeft = makeSector(hpi, 2.0f * hpi, radius * 2.0f, points / 4, origin + v2{-halfSize.x, halfSize.y}, colour);
	topLeft.vertices[0].texCoord = {radius / size.x, radius / size.y};
	sectorUVs(topLeft.vertices);
	auto bottomLeft = makeSector(2.0f * hpi, 3.0f * hpi, radius * 2.0f, points / 4, origin - halfSize, colour);
	bottomLeft.vertices[0].texCoord = {radius / size.x, 1.0f - radius / size.y};
	sectorUVs(bottomLeft.vertices);
	auto bottomRight = makeSector(3.0f * hpi, 4.0f * hpi, radius * 2.0f, points / 4, origin + v2{halfSize.x, -halfSize.y}, colour);
	bottomRight.vertices[0].texCoord = {1.0f - radius / size.x, 1.0f - radius / size.y};
	sectorUVs(bottomRight.vertices);
	append(left, right, topRight, topLeft, bottomLeft, bottomRight, centre);
	return left;
} // namespace le

void graphics::append(graphics::Geometry& out_dst, graphics::Geometry const& src) {
	out_dst.vertices.reserve(out_dst.vertices.size() + src.vertices.size());
	out_dst.indices.reserve(out_dst.indices.size() + src.indices.size());
	u32 offset = (u32)out_dst.vertices.size();
	std::copy(src.vertices.begin(), src.vertices.end(), std::back_inserter(out_dst.vertices));
	for (u32 const index : src.indices) { out_dst.indices.push_back(index + offset); }
}
} // namespace le
