#pragma once
#include <vector>
#include <core/colour.hpp>
#include <core/span.hpp>
#include <core/std_types.hpp>
#include <graphics/basis.hpp>

namespace le::graphics {
enum class VertType { ePosCol, ePosColUV, ePosColNormUV };

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

	void reserve(u32 vertCount, u32 indexCount);
	u32 addVertex(Vert<V> const& vertex);
	void addIndices(View<u32> newIndices);

	std::vector<glm::vec3> positions() const;
};

struct Albedo final {
	Colour ambient = colours::white;
	Colour diffuse = colours::white;
	Colour specular = colours::white;
};

Geometry makeQuad(glm::vec2 size = {1.0f, 1.0f}, glm::vec2 origin = {0.0f, 0.0f});
Geometry makeCube(f32 side = 1.0f);
Geometry makeCircle(f32 diameter = 1.0f, u16 points = 16);
Geometry makeCone(f32 diam = 1.0f, f32 height = 1.0f, u16 points = 16);
Geometry makeCubedSphere(f32 diameter, u8 quadsPerSide);

// impl

template <VertType V>
void Geom<V>::reserve(u32 vertCount, u32 indexCount) {
	vertices.reserve(vertCount);
	indices.reserve(indexCount);
}

template <VertType V>
u32 Geom<V>::addVertex(Vert<V> const& vertex) {
	u32 ret = (u32)vertices.size();
	vertices.push_back(vertex);
	return ret;
}

template <VertType V>
void Geom<V>::addIndices(View<u32> newIndices) {
	std::copy(newIndices.begin(), newIndices.end(), std::back_inserter(indices));
}

template <VertType V>
std::vector<glm::vec3> Geom<V>::positions() const {
	std::vector<glm::vec3> ret;
	for (auto const& v : vertices) { ret.push_back(v.position); }
	return ret;
}
} // namespace le::graphics
