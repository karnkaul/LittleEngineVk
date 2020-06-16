#include <algorithm>
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/utils.hpp>
#include <engine/gfx/mesh.hpp>
#include <engine/assets/resources.hpp>
#include <gfx/common.hpp>
#include <gfx/deferred.hpp>
#include <gfx/vram.hpp>

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
	bufferInfo.queueFlags = QFlag::eGraphics | QFlag::eTransfer;
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
	m_material = info.material;
	if (!m_material.pMaterial)
	{
		m_material.pMaterial = Resources::inst().get<Material>("materials/default");
	}
	updateGeometry(std::move(info.geometry));
}

Mesh::Mesh(Mesh&&) = default;
Mesh& Mesh::operator=(Mesh&&) = default;

Mesh::~Mesh()
{
	if (m_uImpl)
	{
		if (m_type == Type::eDynamic)
		{
			vram::unmapMemory(m_uImpl->vbo.buffer);
			vram::unmapMemory(m_uImpl->ibo.buffer);
		}
		deferred::release(m_uImpl->vbo.buffer);
		deferred::release(m_uImpl->ibo.buffer);
	}
}

void Mesh::updateGeometry(Geometry geometry)
{
	if (!m_uImpl || geometry.vertices.empty())
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
			if (bHostVisible)
			{
				vram::unmapMemory(m_uImpl->vbo.buffer);
			}
			deferred::release(m_uImpl->vbo.buffer);
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
			if (bHostVisible)
			{
				vram::unmapMemory(m_uImpl->ibo.buffer);
			}
			deferred::release(m_uImpl->ibo.buffer);
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
	m_triCount = iSize > 0 ? (u64)m_uImpl->ibo.count / 3 : (u64)m_uImpl->vbo.count / 3;
	return;
}

Asset::Status Mesh::update()
{
	if (!m_uImpl)
	{
		m_status = Status::eMoved;
		return m_status;
	}
	if (m_status == Status::eLoading)
	{
		auto const vboState = utils::futureState(m_uImpl->vbo.copied);
		auto const iboState = utils::futureState(m_uImpl->ibo.copied);
		if (vboState == FutureState::eReady && (m_uImpl->ibo.count == 0 || iboState == FutureState::eReady))
		{
			m_status = Status::eReady;
		}
	}
	return m_status;
}
} // namespace le::gfx
