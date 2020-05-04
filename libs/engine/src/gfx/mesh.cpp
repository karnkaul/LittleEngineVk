#include <algorithm>
#include "core/assert.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include "engine/gfx/mesh.hpp"
#include "engine/assets/resources.hpp"
#include "gfx/common.hpp"
#include "gfx/vram.hpp"
#include "gfx/utils.hpp"

namespace le::gfx
{
namespace
{
Buffer createXBO(std::string_view name, vk::DeviceSize size, vk::BufferUsageFlagBits usage)
{
	BufferInfo bufferInfo;
	bufferInfo.size = size;
	bufferInfo.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
	bufferInfo.usage = usage | vk::BufferUsageFlagBits::eTransferDst;
	bufferInfo.queueFlags = gfx::QFlag::eGraphics | gfx::QFlag::eTransfer;
	bufferInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	bufferInfo.name = name;
	return vram::createBuffer(bufferInfo);
};
} // namespace

Material::Material(stdfs::path id, Info info) : Asset(std::move(id))
{
	m_albedo = info.albedo;
	m_status = Status::eReady;
}

Asset::Status Material::update()
{
	return m_status;
}

Mesh::Mesh(stdfs::path id, Info info) : Asset(std::move(id))
{
	auto const idStr = m_id.generic_string();
	m_uImpl = std::make_unique<MeshImpl>();
	ASSERT(!info.geometry.vertices.empty(), "No vertex data!");
	m_material = info.material;
	if (!m_material.pMaterial)
	{
		m_material.pMaterial = g_pResources->get<Material>("materials/default");
	}
	updateGeometry(std::move(info.geometry));
}

Mesh::~Mesh()
{
	vram::release(m_uImpl->vbo.buffer, m_uImpl->ibo.buffer);
}

void Mesh::updateGeometry(Geometry geometry)
{
	if (geometry.vertices.empty())
	{
		return;
	}
	auto const idStr = m_id.generic_string();
	auto const vSize = (vk::DeviceSize)geometry.vertices.size() * sizeof(Vertex);
	auto const iSize = (vk::DeviceSize)geometry.indices.size() * sizeof(u32);
	if (vSize > m_uImpl->vbo.buffer.writeSize)
	{
		if (m_uImpl->vbo.buffer.writeSize > 0)
		{
			m_uImpl->oldVbo.push_back(std::move(m_uImpl->vbo));
		}
		std::string const name = idStr + "_VBO";
		m_uImpl->vbo.buffer = createXBO(name, vSize, vk::BufferUsageFlagBits::eVertexBuffer);
	}
	if (iSize > m_uImpl->ibo.buffer.writeSize)
	{
		if (m_uImpl->ibo.buffer.writeSize > 0)
		{
			m_uImpl->oldIbo.push_back(std::move(m_uImpl->ibo));
		}
		std::string const name = idStr + "_IBO";
		m_uImpl->ibo.buffer = createXBO(name, iSize, vk::BufferUsageFlagBits::eIndexBuffer);
	}
	m_uImpl->vbo.copied = vram::stage(m_uImpl->vbo.buffer, geometry.vertices.data(), vSize);
	if (!geometry.indices.empty())
	{
		m_uImpl->ibo.copied = vram::stage(m_uImpl->ibo.buffer, geometry.indices.data(), iSize);
	}
	m_uImpl->vertexCount = (u32)geometry.vertices.size();
	m_uImpl->indexCount = (u32)geometry.indices.size();
	m_status = Status::eLoading;
	return;
}

Asset::Status Mesh::update()
{
	if (m_status == Status::eLoading)
	{
		if (allSignalled({m_uImpl->vbo.copied, m_uImpl->ibo.copied}))
		{
			m_status = Status::eReady;
		}
	}
	auto removeCopied = [](MeshImpl::Data const& data) -> bool {
		if (isSignalled(data.copied))
		{
			vram::release(data.buffer);
			return true;
		}
		return false;
	};
	m_uImpl->oldVbo.erase(std::remove_if(m_uImpl->oldVbo.begin(), m_uImpl->oldVbo.end(), removeCopied), m_uImpl->oldVbo.end());
	m_uImpl->oldIbo.erase(std::remove_if(m_uImpl->oldIbo.begin(), m_uImpl->oldIbo.end(), removeCopied), m_uImpl->oldIbo.end());
	return m_status;
}
} // namespace le::gfx
