#pragma once
#include <vector>
#include "core/assert.hpp"
#include "core/flags.hpp"
#include "core/utils.hpp"
#include "engine/gfx/mesh.hpp"
#include "gfx/common.hpp"
#include "gfx/vram.hpp"

namespace le::gfx
{
class Texture;

namespace rd
{
struct UBOView final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	alignas(16) glm::mat4 mat_vp;
	alignas(16) glm::mat4 mat_v;
	alignas(16) glm::mat4 mat_p;
	alignas(16) glm::mat4 mat_ui;
	alignas(16) glm::vec3 pos_v;
};

struct SSBOModels final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	std::vector<glm::mat4> ssbo;
};

struct SSBONormals final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	std::vector<glm::mat4> ssbo;
};

struct SSBOMaterials final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	struct Mat final
	{
		alignas(16) glm::vec4 ambient;
		alignas(16) glm::vec4 diffuse;
		alignas(16) glm::vec4 specular;
		alignas(16) f32 shininess;

		Mat() = default;
		Mat(Material const& material);
	};

	std::vector<Mat> ssbo;
};

struct SSBOTints final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	std::vector<glm::vec4> ssbo;
};

struct SSBOFlags final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	enum : u32
	{
		eTEXTURED = 1 << 0,
		eLIT = 1 << 1,
	};

	std::vector<u32> ssbo;
};

struct SSBOs final
{
	SSBOModels models;
	SSBONormals normals;
	SSBOMaterials materials;
	SSBOTints tints;
	SSBOFlags flags;
};

struct Textures final
{
	constexpr static u32 max = 1024;

	static vk::DescriptorSetLayoutBinding const s_diffuseLayoutBinding;
	static vk::DescriptorSetLayoutBinding const s_specularLayoutBinding;
};

struct WriteInfo final
{
	vk::DescriptorSet set;
	vk::DescriptorType type;
	vk::DescriptorBufferInfo* pBuffer = nullptr;
	vk::DescriptorImageInfo* pImage = nullptr;
	u32 binding = 0;
	u32 arrayElement = 0;
	u32 count = 1;
};

struct ShaderWriter final
{
	vk::DescriptorType type;
	u32 binding = 0;

	void write(vk::DescriptorSet set, Buffer const& buffer, u32 idx) const;
	void write(vk::DescriptorSet set, Texture const& texture, u32 idx) const;
};

template <typename T>
class UBOHandle final
{
public:
	struct Buf
	{
		Buffer buffer;
		std::vector<vk::Fence> inUse;
	};
	Buf m_buf;
	ShaderWriter m_writer;
	vk::BufferUsageFlags m_usage;
	u32 m_arraySize;

	UBOHandle(vk::BufferUsageFlags usage) : m_usage(usage) {}

	void create()
	{
		u32 const size = (u32)sizeof(T);
		if (m_buf.buffer.writeSize < size)
		{
			if (!m_buf.inUse.empty())
			{
				waitAll(m_buf.inUse);
				m_buf.inUse.clear();
			}
			BufferInfo info;
			info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			info.queueFlags = QFlag::eGraphics;
			info.usage = m_usage;
			info.size = size;
			info.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
#if defined(LEVK_VKRESOURCE_NAMES)
			info.name = utils::tName<T>();
#endif
			m_buf.buffer = vram::createBuffer(info);
		}
		return;
	}

	void release()
	{
		waitAll(m_buf.inUse);
		vram::release(m_buf.buffer);
		m_buf.buffer = Buffer();
		m_buf.inUse.clear();
		return;
	}

	bool write(T const& data, vk::DescriptorSet set)
	{
		create();
		if (!vram::write(m_buf.buffer, &data))
		{
			return false;
		}
		m_writer.write(set, m_buf.buffer, 0);
		return true;
	}
};

template <typename T>
class SSBOHandle final
{
public:
	struct Buf
	{
		Buffer buffer;
		std::vector<vk::Fence> inUse;
	};

public:
	Buf m_buf;
	ShaderWriter m_writer;
#if defined(LEVK_VKRESOURCE_NAMES)
	std::string m_bufferName;
#endif
	vk::BufferUsageFlags m_usage;
	std::vector<Buf> m_pending;
	u32 m_arraySize = 1;

public:
	SSBOHandle(vk::BufferUsageFlagBits usage, [[maybe_unused]] std::string name)
		:
#if defined(LEVK_VKRESOURCE_NAMES)
		  m_bufferName(std::move(name)),
#endif
		  m_usage(usage)
	{
	}

public:
	void release()
	{
		waitAll(m_buf.inUse);
		vram::release(m_buf.buffer);
		for (auto& buf : m_pending)
		{
			waitAll(buf.inUse);
			vram::release(buf.buffer);
		}
		m_buf.buffer = Buffer();
		m_buf.inUse.clear();
		return;
	}

	void update()
	{
		for (auto iter = m_pending.begin(); iter != m_pending.end();)
		{
			auto& buf = *iter;
			if (allSignalled(buf.inUse))
			{
				vram::release(buf.buffer);
				iter = m_pending.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

	bool write(T const& ssbo, vk::DescriptorSet set)
	{
		m_arraySize = (u32)ssbo.ssbo.size();
		ASSERT(m_arraySize > 0, "Empty buffer!");
		u32 const tSize = (u32)(sizeof(ssbo.ssbo.at(0)));
		create(tSize);
		if (!vram::write(m_buf.buffer, ssbo.ssbo.data(), (vk::DeviceSize)(ssbo.ssbo.size() * tSize)))
		{
			return false;
		}
		m_writer.write(set, m_buf.buffer, 0);
		return true;
	}

private:
	void create(u32 tSize)
	{
		u32 const size = tSize * m_arraySize;
		if (m_buf.buffer.writeSize < size)
		{
			if (!m_buf.inUse.empty())
			{
				m_pending.push_back(std::move(m_buf));
			}
			BufferInfo info;
			info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			info.queueFlags = QFlag::eGraphics;
			info.usage = m_usage;
			info.size = size;
			info.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
#if defined(LEVK_VKRESOURCE_NAMES)
			info.name = m_bufferName;
#endif
			m_buf.buffer = vram::createBuffer(info);
		}
		return;
	}
};

class Set final
{
public:
	vk::DescriptorSet m_descriptorSet;

private:
	UBOHandle<UBOView> m_view;
	SSBOHandle<SSBOModels> m_models;
	SSBOHandle<SSBONormals> m_normals;
	SSBOHandle<SSBOMaterials> m_materials;
	SSBOHandle<SSBOTints> m_tints;
	SSBOHandle<SSBOFlags> m_flags;
	ShaderWriter m_diffuse;
	ShaderWriter m_specular;

public:
	Set();

public:
	void initSSBOs();
	void update();
	void attach(vk::Fence drawing);
	void destroy();

public:
	void writeView(UBOView const& view);
	void writeSSBOs(SSBOs const& ssbos);
	void writeDiffuse(Texture const& diffuse, u32 idx);
	void writeSpecular(Texture const& specular, u32 idx);

	void resetTextures();
};

struct SetLayouts final
{
	vk::DescriptorPool descriptorPool;
	std::vector<Set> set;
};

inline vk::DescriptorSetLayout g_setLayout;

void init();
void deinit();

SetLayouts allocateSets(u32 copies);
} // namespace rd
} // namespace le::gfx
