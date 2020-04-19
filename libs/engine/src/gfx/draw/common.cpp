#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/map_store.hpp"
#include "engine/gfx/draw/common.hpp"
#include "gfx/utils.hpp"
#include "gfx/vram.hpp"
#include "common_impl.hpp"

namespace le
{
namespace gfx
{
vk::VertexInputBindingDescription vbo::bindingDescription()
{
	vk::VertexInputBindingDescription ret;
	ret.binding = vertexBinding;
	ret.stride = sizeof(Vertex);
	ret.inputRate = vk::VertexInputRate::eVertex;
	return ret;
}

std::vector<vk::VertexInputAttributeDescription> vbo::attributeDescriptions()
{
	std::vector<vk::VertexInputAttributeDescription> ret;
	vk::VertexInputAttributeDescription pos;
	pos.binding = vertexBinding;
	pos.location = 0;
	pos.format = vk::Format::eR32G32B32Sfloat;
	pos.offset = offsetof(Vertex, position);
	ret.push_back(pos);
	vk::VertexInputAttributeDescription col;
	col.binding = vertexBinding;
	col.location = 1;
	col.format = vk::Format::eR32G32B32Sfloat;
	col.offset = offsetof(Vertex, colour);
	ret.push_back(col);
	vk::VertexInputAttributeDescription uv;
	uv.binding = vertexBinding;
	uv.location = 2;
	uv.format = vk::Format::eR32G32Sfloat;
	uv.offset = offsetof(Vertex, texCoord);
	ret.push_back(uv);
	return ret;
}
} // namespace gfx

namespace
{
TMapStore<std::unordered_map<std::string, gfx::Mesh>> g_meshes;
gfx::MeshImpl::ID g_nextMeshID;
} // namespace

bool gfx::Mesh::isReady() const
{
	return allSignalled({uImpl->vboCopied, uImpl->iboCopied});
}

gfx::Mesh* gfx::loadMesh(stdfs::path const& id, Geometry const& geometry)
{
	auto const idStr = id.generic_string();
	bool const bExists = g_meshes.m_map.find(idStr) != g_meshes.m_map.end();
	ASSERT(!bExists, "ID already loaded!");
	if (!bExists)
	{
		Mesh newMesh;
		newMesh.id = id;
		newMesh.uImpl = std::make_unique<MeshImpl>();
		newMesh.uImpl->meshID = ++g_nextMeshID.handle;
		BufferInfo info;
		info.size = (u32)geometry.vertices.size() * sizeof(Vertex);
		info.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
		info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		info.queueFlags = gfx::QFlag::eGraphics | gfx::QFlag::eTransfer;
		info.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		info.name = idStr + "_VBO";
		newMesh.uImpl->vbo = vram::createBuffer(info);
		newMesh.uImpl->vboCopied = vram::stage(newMesh.uImpl->vbo, geometry.vertices.data());
		if (!geometry.indices.empty())
		{
			info.size = (u32)geometry.indices.size() * sizeof(u32);
			info.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
			info.name = idStr + "_IBO";
			newMesh.uImpl->ibo = vram::createBuffer(info);
			newMesh.uImpl->iboCopied = vram::stage(newMesh.uImpl->ibo, geometry.indices.data());
		}
		newMesh.uImpl->vertexCount = (u32)geometry.vertices.size();
		newMesh.uImpl->indexCount = (u32)geometry.indices.size();
		g_meshes.insert(idStr, std::move(newMesh));
		return getMesh(id);
	}
	return nullptr;
}

gfx::Mesh* gfx::getMesh(stdfs::path const& id)
{
	auto [pMesh, bResult] = g_meshes.get(id.generic_string());
	if (bResult && pMesh)
	{
		return pMesh;
	}
	return nullptr;
}

bool gfx::unloadMesh(Mesh* pMesh)
{
	if (pMesh)
	{
		vram::release(pMesh->uImpl->vbo, pMesh->uImpl->ibo);
		return g_meshes.unload(pMesh->id.generic_string());
	}
	return false;
}

void gfx::unloadAllMeshes()
{
	for (auto& [id, mesh] : g_meshes.m_map)
	{
		vram::release(mesh.uImpl->vbo, mesh.uImpl->ibo);
	}
	g_meshes.unloadAll();
	return;
}
} // namespace le
