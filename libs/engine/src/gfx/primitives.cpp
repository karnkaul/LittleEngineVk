#include <functional>
#include <glm/gtx/rotate_vector.hpp>
#include "core/assert.hpp"
#include "engine/gfx/primitives.hpp"

namespace le
{
gfx::Geometry gfx::createQuad(f32 side /* = 1.0f */)
{
	Geometry ret;
	f32 const h = side * 0.5f;
	// clang-format off
	ret.vertices = {
		{{-h, -h, 0.0f}, {}, {}, {1.0f, 1.0f}},
		{{ h, -h, 0.0f}, {}, {}, {0.0f, 1.0f}},
		{{ h,  h, 0.0f}, {}, {}, {0.0f, 0.0f}},
		{{-h,  h, 0.0f}, {}, {}, {1.0f, 0.0f}}
	};
	// clang-format on
	ret.indices = {0, 1, 2, 2, 3, 0};
	return ret;
}

gfx::Geometry gfx::createCube(f32 side /* = 1.0f */)
{
	Geometry ret;
	f32 const s = side * 0.5f;
	// clang-format off
	ret.vertices = {
		// front
		{{-s, -s,  s}, {}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
		{{ s, -s,  s}, {}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
		{{ s,  s,  s}, {}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
		{{-s,  s,  s}, {}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},
		// back
		{{-s, -s, -s}, {}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
		{{ s, -s, -s}, {}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
		{{ s,  s, -s}, {}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
		{{-s,  s, -s}, {}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
		// left
		{{-s,  s,  s}, {}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
		{{-s,  s, -s}, {}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
		{{-s, -s, -s}, {}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
		{{-s, -s,  s}, {}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
		// right
		{{ s,  s,  s}, {}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
		{{ s,  s, -s}, {}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0}},
		{{ s, -s, -s}, {}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
		{{ s, -s,  s}, {}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
		// down
		{{-s, -s, -s}, {}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
		{{ s, -s, -s}, {}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
		{{ s, -s,  s}, {}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
		{{-s, -s,  s}, {}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
		//up
		{{-s,  s, -s}, {}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},
		{{ s,  s, -s}, {}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
		{{ s,  s,  s}, {}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
		{{-s,  s,  s}, {}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}}
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

gfx::Geometry gfx::createCircle(f32 diameter, u16 points)
{
	ASSERT(points < 1000, "Max points is 1000");
	f32 const r = diameter * 0.5f;
	Geometry ret;
	f32 const arc = 360.0f / points;
	glm::vec3 const norm(0.0f, 0.0f, 1.0f);
	ret.reserve(1 + 1 + (u32)points, (1 + (u32)points) * 4);
	u32 const iCentre = ret.addVertex({{}, {}, norm, {0.5f, 0.5f}});
	for (s32 i = 0; i <= points; ++i)
	{
		f32 const x1 = glm::cos(glm::radians(arc * i));
		f32 const y1 = glm::sin(glm::radians(arc * i));
		f32 const s1 = (x1 + 1.0f) * 0.5f;
		f32 const t1 = (y1 + 1.0f) * 0.5f;
		u32 const iv1 = ret.addVertex({{r * x1, r * y1, 0.0f}, {}, norm, {s1, t1}});
		if (i > 0)
		{
			ret.addIndices({iCentre, iv1 - 1, iv1});
		}
	}
	return ret;
}

gfx::Geometry gfx::createCubedSphere(f32 diam, u8 quadsPerSide)
{
	ASSERT(quadsPerSide < 30, "Max quads per side is 30");
	Geometry ret;
	u32 qCount = (u32)(quadsPerSide * quadsPerSide);
	ret.reserve(qCount * 4 * 6, qCount * 6 * 6);
	glm::vec3 const bl(-1.0f, -1.0f, 1.0f);
	std::vector<std::pair<glm::vec3, glm::vec2>> points;
	points.reserve(4 * qCount);
	f32 const s = 2.0f / quadsPerSide;
	f32 const duv = 1.0f / quadsPerSide;
	f32 u = 0.0f;
	f32 v = 0.0f;
	for (s32 row = 0; row < quadsPerSide; ++row)
	{
		v = row * duv;
		for (s32 col = 0; col < quadsPerSide; ++col)
		{
			u = col * duv;
			glm::vec3 const o = s * glm::vec3((f32)col, (f32)row, 0.0f);
			points.push_back(std::make_pair(glm::vec3(bl + o), glm::vec2(u, 1.0f - v)));
			points.push_back(std::make_pair(glm::vec3(bl + glm::vec3(s, 0.0f, 0.0f) + o), glm::vec2(u + duv, 1.0 - v)));
			points.push_back(std::make_pair(glm::vec3(bl + glm::vec3(s, s, 0.0f) + o), glm::vec2(u + duv, 1.0f - v - duv)));
			points.push_back(std::make_pair(glm::vec3(bl + glm::vec3(0.0f, s, 0.0f) + o), glm::vec2(u, 1.0f - v - duv)));
		}
	}
	auto addSide = [&points, &ret, diam](std::function<glm::vec3(glm::vec3 const&)> transform) {
		s32 idx = 0;
		std::vector<u32> iV;
		iV.reserve(4);
		for (auto const& p : points)
		{
			if (iV.size() == 4)
			{
				ret.addIndices({iV[0], iV[1], iV[2], iV[2], iV[3], iV[0]});
				iV.clear();
			}
			++idx;
			auto const pt = transform(p.first) * diam * 0.5f;
			iV.push_back(ret.addVertex({pt, {}, pt, p.second}));
		}
		if (iV.size() == 4)
		{
			ret.addIndices({iV[0], iV[1], iV[2], iV[2], iV[3], iV[0]});
		}
	};
	addSide([](auto const& p) { return glm::normalize(p); });
	addSide([](auto const& p) { return glm::normalize(glm::rotate(p, glm::radians(180.0f), g_nUp)); });
	addSide([](auto const& p) { return glm::normalize(glm::rotate(p, glm::radians(90.0f), g_nUp)); });
	addSide([](auto const& p) { return glm::normalize(glm::rotate(p, glm::radians(-90.0f), g_nUp)); });
	addSide([](auto const& p) { return glm::normalize(glm::rotate(p, glm::radians(90.0f), g_nRight)); });
	addSide([](auto const& p) { return glm::normalize(glm::rotate(p, glm::radians(-90.0f), g_nRight)); });
	return ret;
}
} // namespace le
