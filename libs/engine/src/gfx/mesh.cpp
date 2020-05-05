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
Buffer createXBO(std::string_view name, vk::DeviceSize size, vk::BufferUsageFlagBits usage, bool bHostVisible)
{
	BufferInfo bufferInfo;
	bufferInfo.size = size;
	if (bHostVisible)
	{
		bufferInfo.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		bufferInfo.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	}
	else
	{
		bufferInfo.properties = vk::MemoryPropertyFlagBits::eDeviceLocal;
		bufferInfo.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY;
	}
	bufferInfo.usage = usage | vk::BufferUsageFlagBits::eTransferDst;
	bufferInfo.queueFlags = gfx::QFlag::eGraphics | gfx::QFlag::eTransfer;
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

Mesh::Mesh(stdfs::path id, Info info) : Asset(std::move(id)), m_type(info.type)
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
	if (m_type == Type::eDynamic)
	{
		if (m_uImpl->vbo.pMem)
		{
			vram::unmapMemory(m_uImpl->vbo.buffer);
		}
		if (m_uImpl->ibo.pMem)
		{
			vram::unmapMemory(m_uImpl->ibo.buffer);
		}
	}
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
	auto const bHostVisible = m_type == Type::eDynamic;
	if (vSize > m_uImpl->vbo.buffer.writeSize)
	{
		if (m_uImpl->vbo.buffer.writeSize > 0)
		{
			m_uImpl->toRelease.push_back(std::move(m_uImpl->vbo));
			m_uImpl->vbo = {};
		}
		std::string const name = idStr + "_VBO";
		m_uImpl->vbo.buffer = createXBO(name, vSize, vk::BufferUsageFlagBits::eVertexBuffer, bHostVisible);
		if (bHostVisible)
		{
			m_uImpl->vbo.pMem = vram::mapMemory(m_uImpl->vbo.buffer, vSize);
		}
	}
	if (iSize > m_uImpl->ibo.buffer.writeSize)
	{
		if (m_uImpl->ibo.buffer.writeSize > 0)
		{
			m_uImpl->toRelease.push_back(std::move(m_uImpl->ibo));
			m_uImpl->ibo = {};
		}
		std::string const name = idStr + "_IBO";
		m_uImpl->ibo.buffer = createXBO(name, iSize, vk::BufferUsageFlagBits::eIndexBuffer, bHostVisible);
		if (bHostVisible)
		{
			m_uImpl->ibo.pMem = vram::mapMemory(m_uImpl->ibo.buffer, iSize);
		}
	}
	switch (m_type)
	{
	case Type::eStatic:
	{
		m_uImpl->vbo.copied = vram::stage(m_uImpl->vbo.buffer, geometry.vertices.data(), vSize);
		if (!geometry.indices.empty())
		{
			m_uImpl->ibo.copied = vram::stage(m_uImpl->ibo.buffer, geometry.indices.data(), iSize);
		}
		m_status = Status::eLoading;
		break;
	}
	case Type::eDynamic:
	{
		std::memcpy(m_uImpl->vbo.pMem, geometry.vertices.data(), vSize);
		if (!geometry.indices.empty())
		{
			std::memcpy(m_uImpl->ibo.pMem, geometry.indices.data(), iSize);
		}
		break;
	}
	default:
		break;
	}
	m_uImpl->vbo.count = (u32)geometry.vertices.size();
	m_uImpl->ibo.count = (u32)geometry.indices.size();
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
	auto removeCopied = [this](MeshImpl::Data const& data) -> bool {
		if (isSignalled(data.copied))
		{
			if (m_type == Type::eDynamic)
			{
				vram::unmapMemory(data.buffer);
			}
			vram::release(data.buffer);
			return true;
		}
		return false;
	};
	m_uImpl->toRelease.erase(std::remove_if(m_uImpl->toRelease.begin(), m_uImpl->toRelease.end(), removeCopied), m_uImpl->toRelease.end());
	return m_status;
}
} // namespace le::gfx
