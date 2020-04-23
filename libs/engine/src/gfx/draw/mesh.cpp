#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/gfx/draw/mesh.hpp"
#include "gfx/common.hpp"
#include "gfx/vram.hpp"
#include "gfx/utils.hpp"

namespace le::gfx
{
Mesh::Mesh(stdfs::path id, Info info) : Asset(std::move(id))
{
	auto const idStr = m_id.generic_string();
	m_uImpl = std::make_unique<MeshImpl>();
	ASSERT(!info.geometry.vertices.empty(), "No vertex data!");
	if (info.geometry.vertices.empty())
	{
		LOG_E("[{}] [{}] Error: No vertex data!", utils::tName<Mesh>(), idStr);
		m_status = Status::eError;
	}
	else
	{
		BufferInfo bufferInfo;
		bufferInfo.size = (u32)info.geometry.vertices.size() * sizeof(Vertex);
		bufferInfo.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
		bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
		bufferInfo.queueFlags = gfx::QFlag::eGraphics | gfx::QFlag::eTransfer;
		bufferInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
		bufferInfo.name = idStr + "_VBO";
		m_uImpl->vbo = vram::createBuffer(bufferInfo);
		m_uImpl->vboCopied = vram::stage(m_uImpl->vbo, info.geometry.vertices.data());
		if (!info.geometry.indices.empty())
		{
			bufferInfo.size = (u32)info.geometry.indices.size() * sizeof(u32);
			bufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
			bufferInfo.name = idStr + "_IBO";
			m_uImpl->ibo = vram::createBuffer(bufferInfo);
			m_uImpl->iboCopied = vram::stage(m_uImpl->ibo, info.geometry.indices.data());
		}
		m_uImpl->vertexCount = (u32)info.geometry.vertices.size();
		m_uImpl->indexCount = (u32)info.geometry.indices.size();
		m_status = Status::eLoading;
	}
}

Mesh::~Mesh()
{
	vram::release(m_uImpl->vbo, m_uImpl->ibo);
}

Asset::Status Mesh::update()
{
	if (m_status == Status::eLoading)
	{
		if (allSignalled({m_uImpl->vboCopied, m_uImpl->iboCopied}))
		{
			m_status = Status::eReady;
		}
	}
	return m_status;
}
} // namespace le::gfx
