#pragma once
#include <vector>
#include <core/colour.hpp>
#include <core/span.hpp>
#include <core/std_types.hpp>
#include <graphics/basis.hpp>

namespace le::graphics {
enum class VertType { ePosCol, ePosColUV, ePosColNormUV };
enum class Topology { eLineList = 1, eTriangleList = 2 };

template <VertType V>
struct Vert;
template <VertType V>
struct Geom;

using Vertex = Vert<VertType::ePosColNormUV>;
using Geometry = Geom<VertType::ePosColNormUV>;

template <>
struct Vert<VertType::ePosCol> {
	glm::vec3 position = {};
	glm::vec3 colour = glm::vec3(1.0f);
};
template <>
struct Vert<VertType::ePosColUV> {
	glm::vec3 position = {};
	glm::vec3 colour = glm::vec3(1.0f);
	glm::vec2 texCoord = {};
};
template <>
struct Vert<VertType::ePosColNormUV> {
	glm::vec3 position = {};
	glm::vec3 colour = glm::vec3(1.0f);
	glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec2 texCoord = {};
};

template <VertType V>
struct Geom {
	std::vector<Vert<V>> vertices;
	std::vector<u32> indices;

	Geom& reserve(u32 vertCount, u32 indexCount);
	u32 addVertex(Vert<V> const& vertex);
	Geom& addIndices(Span<u32 const> newIndices);
	Geom& autoIndex(Topology topology, u32 loopback = 0);
	Geom& offset(glm::vec3 offset) noexcept;
	Geom& append(Geom const& in);

	template <typename... T>
		requires((sizeof...(T) > 1) && (std::is_same_v<T, Geometry> && ...))
	Geom& append(T const&... src) {
		(append(src), ...);
		return *this;
	}

	std::vector<glm::vec3> positions() const;
};

struct Albedo final {
	Colour ambient = colours::white;
	Colour diffuse = colours::white;
	Colour specular = colours::white;
};

struct UVQuad {
	glm::vec2 topLeft = {0.0f, 0.0f};
	glm::vec2 bottomRight = {1.0f, 1.0f};
};

struct GeomInfo {
	glm::vec3 origin{};
	glm::vec3 colour = glm::vec3(1.0f);
};

Geometry makeQuad(glm::vec2 size = {1.0f, 1.0f}, GeomInfo const& info = {}, UVQuad const& uv = {}, Topology topo = Topology::eTriangleList);
Geometry makeSector(glm::vec2 radExtent, f32 diameter, u16 points, GeomInfo const& info = {});
Geometry makeCircle(f32 diameter = 1.0f, u16 points = 32, GeomInfo const& info = {});
Geometry makeCone(f32 diam = 1.0f, f32 height = 1.0f, u16 points = 32, GeomInfo const& info = {});
Geometry makeCube(f32 side = 1.0f, GeomInfo const& info = {}, Topology topo = Topology::eTriangleList);
Geometry makeCubedSphere(f32 diameter = 1.0f, u8 quadsPerSide = 8, GeomInfo const& info = {});
Geometry makeRoundedQuad(glm::vec2 size = {1.0f, 1.0f}, f32 radius = 0.25f, u16 points = 32, GeomInfo const& info = {});

struct IndexStitcher {
	std::vector<u32>& indices;
	u32 start;
	u32 stride;
	u32 pin;

	IndexStitcher(std::vector<u32>& indices, u32 start, u32 stride) noexcept : indices(indices), start(start), stride(stride), pin(start) {}

	void add(u32 index);
	void reset(u32 index);
};

// impl

template <VertType V>
Geom<V>& Geom<V>::reserve(u32 vertCount, u32 indexCount) {
	vertices.reserve(vertCount);
	indices.reserve(indexCount);
	return *this;
}

template <VertType V>
u32 Geom<V>::addVertex(Vert<V> const& vertex) {
	u32 ret = (u32)vertices.size();
	vertices.push_back(vertex);
	return ret;
}

template <VertType V>
Geom<V>& Geom<V>::addIndices(Span<u32 const> newIndices) {
	std::copy(newIndices.begin(), newIndices.end(), std::back_inserter(indices));
	return *this;
}

template <VertType V>
std::vector<glm::vec3> Geom<V>::positions() const {
	std::vector<glm::vec3> ret;
	for (auto const& v : vertices) { ret.push_back(v.position); }
	return ret;
}

template <VertType V>
Geom<V>& Geom<V>::autoIndex(Topology topology, u32 loopback) {
	indices.clear();
	if (!vertices.empty()) {
		IndexStitcher indexer{indices, 0, u32(topology)};
		for (u32 i = 0; i < u32(vertices.size()); ++i) {
			if (loopback > 0 && i > 0 && i % loopback == 0) { indexer.reset(i); }
			indexer.add(i);
		}
		if (loopback > 0) { indexer.reset(0); }
	}
	return *this;
}

template <VertType V>
Geom<V>& Geom<V>::offset(glm::vec3 offset) noexcept {
	for (Vert<V>& vert : vertices) { vert.position += offset; }
	return *this;
}

template <VertType V>
Geom<V>& Geom<V>::append(Geom const& in) {
	vertices.reserve(vertices.size() + in.vertices.size());
	indices.reserve(indices.size() + in.indices.size());
	u32 offset = (u32)vertices.size();
	std::copy(in.vertices.begin(), in.vertices.end(), std::back_inserter(vertices));
	for (u32 const index : in.indices) { indices.push_back(index + offset); }
	return *this;
}
} // namespace le::graphics
