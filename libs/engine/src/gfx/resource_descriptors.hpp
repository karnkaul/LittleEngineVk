#pragma once
#include <deque>
#include <vector>
#include "core/assert.hpp"
#include "core/flags.hpp"
#include "core/utils.hpp"
#include "engine/gfx/mesh.hpp"
#include "engine/gfx/light.hpp"
#include "engine/gfx/renderer.hpp"
#include "gfx/common.hpp"
#include "gfx/deferred.hpp"
#include "gfx/vram.hpp"
#if defined(LEVK_VKRESOURCE_NAMES)
#include "core/utils.hpp"
#endif

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
	alignas(4) u32 dirLightCount;

	UBOView();
	UBOView(Renderer::View const& view, u32 dirLightCount);
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
		alignas(16) glm::vec4 dropColour;
		alignas(16) f32 shininess;

		Mat() = default;
		Mat(Material const& material, Colour dropColour);
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
		eOPAQUE = 1 << 2,
		eDROP_COLOUR = 1 << 3,
		eUI = 1 << 4,
		eSKYBOX = 1 << 5,
	};

	std::vector<u32> ssbo;
};

struct SSBODirLights final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	struct Light
	{
		alignas(16) glm::vec3 ambient;
		alignas(16) glm::vec3 diffuse;
		alignas(16) glm::vec3 specular;
		alignas(16) glm::vec3 direction;

		Light() = default;
		Light(DirLight const& dirLight);
	};

	std::vector<Light> ssbo;
};

struct SSBOs final
{
	SSBOModels models;
	SSBONormals normals;
	SSBOMaterials materials;
	SSBOTints tints;
	SSBOFlags flags;
	SSBODirLights dirLights;
};

struct Textures final
{
	constexpr static u32 max = 1024;

	static vk::DescriptorSetLayoutBinding s_diffuseLayoutBinding;
	static vk::DescriptorSetLayoutBinding s_specularLayoutBinding;
	static vk::DescriptorSetLayoutBinding const s_cubemapLayoutBinding;

	static u32 total();
	static void clampDiffSpecCount(u32 count);
};

struct PushConstants final
{
	u32 objectID = 0;
	u32 diffuseID = 0;
	u32 specularID = 0;
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

	void write(vk::DescriptorSet set, Buffer const& buffer) const;
	void write(vk::DescriptorSet set, std::vector<TextureImpl const*> const& textures) const;
};

template <typename T>
class GPUBuffer
{
public:
	Buffer m_buffer;
	ShaderWriter m_writer;
#if defined(LEVK_VKRESOURCE_NAMES)
	std::string m_bufferName;
#endif
	vk::BufferUsageFlags m_usage;

public:
	GPUBuffer(vk::BufferUsageFlags flags = vk::BufferUsageFlagBits::eStorageBuffer)
		:
#if defined(LEVK_VKRESOURCE_NAMES)
		  m_bufferName(utils::tName<T>()),
#endif
		  m_usage(flags)
	{
		m_writer.binding = T::s_setLayoutBinding.binding;
		m_writer.type = T::s_setLayoutBinding.descriptorType;
	}

public:
	bool writeValue(T const& data, vk::DescriptorSet set)
	{
		create((vk::DeviceSize)sizeof(T));
		if (!vram::write(m_buffer, &data))
		{
			return false;
		}
		m_writer.write(set, m_buffer);
		return true;
	}

	template <typename U>
	bool writeArray(std::vector<U> const& arr, vk::DescriptorSet set)
	{
		ASSERT(arr.size() > 0, "Empty buffer!");
		vk::DeviceSize const size = (vk::DeviceSize)sizeof(U) * (vk::DeviceSize)arr.size();
		create(size);
		if (!vram::write(m_buffer, arr.data(), size))
		{
			return false;
		}
		m_writer.write(set, m_buffer);
		return true;
	}

	void release()
	{
		deferred::release(m_buffer);
		m_buffer = Buffer();
		return;
	}

private:
	void create(vk::DeviceSize size)
	{
		if (m_buffer.writeSize < size)
		{
			deferred::release(m_buffer);
			BufferInfo info;
			info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			info.queueFlags = QFlag::eGraphics;
			info.usage = m_usage;
			info.size = size;
			info.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
#if defined(LEVK_VKRESOURCE_NAMES)
			info.name = m_bufferName;
#endif
			m_buffer = vram::createBuffer(info);
		}
		return;
	}
};

class Set final
{
public:
	vk::DescriptorSet m_descriptorSet;

private:
	GPUBuffer<UBOView> m_view;
	GPUBuffer<SSBOModels> m_models;
	GPUBuffer<SSBONormals> m_normals;
	GPUBuffer<SSBOMaterials> m_materials;
	GPUBuffer<SSBOTints> m_tints;
	GPUBuffer<SSBOFlags> m_flags;
	GPUBuffer<SSBODirLights> m_dirLights;
	ShaderWriter m_diffuse;
	ShaderWriter m_specular;
	ShaderWriter m_cubemap;

public:
	Set();

public:
	void initSSBOs();
	void destroy();

public:
	void writeView(UBOView const& view);
	void writeSSBOs(SSBOs const& ssbos);
	void writeDiffuse(std::deque<Texture const*> const& diffuse);
	void writeSpecular(std::deque<Texture const*> const& specular);
	void writeCubemap(Cubemap const& cubemap);

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
