#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <core/std_types.hpp>

namespace le::gfx
{
constexpr glm::vec3 g_nUp = {0.0f, 1.0f, 0.0f};
constexpr glm::vec3 g_nRight = {1.0f, 0.0f, 0.0f};
constexpr glm::vec3 g_nFront = {0.0f, 0.0f, 1.0f};
constexpr glm::quat g_qIdentity = {1.0f, 0.0f, 0.0f, 0.0f};

struct Vertex final
{
	glm::vec3 position = {};
	glm::vec3 colour = glm::vec3(1.0f);
	glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec2 texCoord = {};
};

struct Geometry final
{
	std::vector<Vertex> vertices;
	std::vector<u32> indices;

	void reserve(u32 vertCount, u32 indexCount);
	u32 addVertex(Vertex const& vertex);
	void addIndices(std::vector<u32> newIndices);
};

Geometry createQuad(f32 side = 1.0f);
Geometry createCube(f32 side = 1.0f);
Geometry createCircle(f32 diameter = 1.0f, u16 points = 16);
Geometry createCone(f32 diam = 1.0f, f32 height = 1.0f, u16 points = 16);
Geometry createCubedSphere(f32 diameter, u8 quadsPerSide);

} // namespace le::gfx
