#pragma once
#include <deque>
#include <vector>
#include "core/assert.hpp"
#include "core/flags.hpp"
#include "core/utils.hpp"
#include "engine/gfx/mesh.hpp"
#include "engine/gfx/light.hpp"
#include "engine/gfx/renderer.hpp"
#include "common.hpp"
#include "deferred.hpp"
#include "vram.hpp"
#if defined(LEVK_VKRESOURCE_NAMES)
#include "core/utils.hpp"
#endif

namespace le::gfx
{
class Texture;

namespace rd
{
// UBO
struct View final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	alignas(16) glm::mat4 mat_vp;
	alignas(16) glm::mat4 mat_v;
	alignas(16) glm::mat4 mat_p;
	alignas(16) glm::mat4 mat_ui;
	alignas(16) glm::vec3 pos_v;
	alignas(4) u32 dirLightCount;

	View();
	View(Renderer::View const& view, u32 dirLightCount);
};

// SSBO
struct Models final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;
	std::vector<glm::mat4> ssbo;
};

// SSBO
struct Normals final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;
	std::vector<glm::mat4> ssbo;
};

// SSBO
struct Materials final
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

// SSBO
struct Tints final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;
	std::vector<glm::vec4> ssbo;
};

// SSBO
struct Flags final
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

// SSBO
struct DirLights final
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

struct ImageSamplers final
{
	static u32 s_max;

	static vk::DescriptorSetLayoutBinding s_diffuseLayoutBinding;
	static vk::DescriptorSetLayoutBinding s_specularLayoutBinding;
	static vk::DescriptorSetLayoutBinding const s_cubemapLayoutBinding;

	static u32 total();
	static void clampDiffSpecCount(u32 hardwareMax);
};

struct StorageBuffers final
{
	Models models;
	Normals normals;
	Materials materials;
	Tints tints;
	Flags flags;
	DirLights dirLights;
};

struct PushConstants final
{
	u32 objectID = 0;
	u32 diffuseID = 0;
	u32 specularID = 0;
};

struct Writer final
{
	vk::DescriptorType type;
	u32 binding = 0;

	void write(vk::DescriptorSet set, Buffer const& buffer) const;
	void write(vk::DescriptorSet set, std::vector<TextureImpl const*> const& textures) const;
};

template <typename T>
class Descriptor final
{
public:
	Buffer m_buffer;
	Writer m_writer;
#if defined(LEVK_VKRESOURCE_NAMES)
	std::string m_bufferName;
#endif
	vk::BufferUsageFlags m_usage;

public:
	Descriptor(vk::BufferUsageFlags flags = vk::BufferUsageFlagBits::eStorageBuffer);

public:
	bool writeValue(T const& data, vk::DescriptorSet set);
	template <typename U>
	bool writeArray(std::vector<U> const& arr, vk::DescriptorSet set);
	void release();

private:
	void create(vk::DeviceSize size);
};

template <>
class Descriptor<ImageSamplers> final
{
public:
	Writer m_writer;

	void writeArray(std::vector<TextureImpl const*> const& textures, vk::DescriptorSet set) const;
};

struct SamplerCounts final
{
	u32 diffuse = ImageSamplers::s_max;
	u32 specular = ImageSamplers::s_max;
};

class Set final
{
public:
	vk::DescriptorPool m_bufferPool;
	vk::DescriptorPool m_samplerPool;
	vk::DescriptorSet m_bufferSet;
	vk::DescriptorSet m_samplerSet;

private:
	Descriptor<View> m_view;
	Descriptor<Models> m_models;
	Descriptor<Normals> m_normals;
	Descriptor<Materials> m_materials;
	Descriptor<Tints> m_tints;
	Descriptor<Flags> m_flags;
	Descriptor<DirLights> m_dirLights;
	Descriptor<ImageSamplers> m_diffuse;
	Descriptor<ImageSamplers> m_specular;
	Descriptor<ImageSamplers> m_cubemap;

public:
	Set();

public:
	void initSSBOs();
	void destroy();
	void resetTextures(SamplerCounts const& counts);

public:
	void writeView(View const& view);
	void writeSSBOs(StorageBuffers const& ssbos);
	void writeDiffuse(std::deque<Texture const*> const& diffuse);
	void writeSpecular(std::deque<Texture const*> const& specular);
	void writeCubemap(Cubemap const& cubemap);
};

struct SetLayouts final
{
	vk::DescriptorSetLayout samplerLayout;
	std::vector<Set> sets;
};

inline vk::DescriptorSetLayout g_bufferLayout;

void init();
void deinit();

SetLayouts allocateSets(u32 copies, SamplerCounts const& samplerCounts = {});

template <typename T>
Descriptor<T>::Descriptor(vk::BufferUsageFlags flags)
	:
#if defined(LEVK_VKRESOURCE_NAMES)
	  m_bufferName(utils::tName<T>()),
#endif
	  m_usage(flags)
{
	m_writer.binding = T::s_setLayoutBinding.binding;
	m_writer.type = T::s_setLayoutBinding.descriptorType;
}

template <typename T>
bool Descriptor<T>::writeValue(T const& data, vk::DescriptorSet set)
{
	create((vk::DeviceSize)sizeof(T));
	if (!vram::write(m_buffer, &data))
	{
		return false;
	}
	m_writer.write(set, m_buffer);
	return true;
}

template <typename T>
template <typename U>
bool Descriptor<T>::writeArray(std::vector<U> const& arr, vk::DescriptorSet set)
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

template <typename T>
void Descriptor<T>::release()
{
	deferred::release(m_buffer);
	m_buffer = Buffer();
	return;
}

template <typename T>
void Descriptor<T>::create(vk::DeviceSize size)
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
} // namespace rd
} // namespace le::gfx
