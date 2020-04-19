#pragma once
#include <filesystem>
#include <memory>
#include <glm/glm.hpp>
#include <vector>
#include "core/std_types.hpp"

namespace stdfs = std::filesystem;

namespace le::gfx
{
struct Vertex final
{
	glm::vec3 position = {};
	glm::vec3 colour = {};
	glm::vec2 texCoord = {};
};

struct Mesh final
{
	stdfs::path id;
	std::unique_ptr<struct MeshImpl> uImpl;

	bool isReady() const;
};

struct Geometry final
{
	std::vector<Vertex> vertices;
	std::vector<u32> indices;
};

Mesh* loadMesh(stdfs::path const& id, Geometry const& geometry);
Mesh* getMesh(stdfs::path const& id);
bool unloadMesh(Mesh* pMesh);
void unloadAllMeshes();
} // namespace le::gfx
