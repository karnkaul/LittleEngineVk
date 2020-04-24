#pragma once
#include <vector>
#include "core/flags.hpp"
#include "core/utils.hpp"
#include "gfx/common.hpp"
#include "gfx/vram.hpp"

namespace le::gfx
{
class Texture;

namespace rd
{
constexpr static u32 maxObjects = 1024;

struct View final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	alignas(16) glm::mat4 mat_vp = glm::mat4(1.0f);
	alignas(16) glm::mat4 mat_v = glm::mat4(1.0f);
	alignas(16) glm::mat4 mat_p = glm::mat4(1.0f);
	alignas(16) glm::mat4 mat_ui = glm::mat4(1.0f);
	alignas(16) glm::vec3 pos_v = glm::vec3(0.0f);
};

struct SSBOModels final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	std::array<glm::mat4, maxObjects> mats_m = {};
};

struct SSBONormals final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	std::array<glm::mat4, maxObjects> mats_n = {};
};

struct SSBOTints final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	std::array<glm::vec4, maxObjects> tints = {};
};

struct SSBOFlags final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	enum : u32
	{
		eTEXTURED = 1 << 0,
		eLIT = 1 << 1,
	};

	std::array<u32, maxObjects> flags = {};
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
struct ShaderBuffer final
{
	Buffer buffer;
	ShaderWriter writer;

	void create(vk::BufferUsageFlags usage)
	{
		if (buffer.buffer == vk::Buffer())
		{
			u32 size = (u32)sizeof(T);
			BufferInfo info;
			info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			info.queueFlags = QFlag::eGraphics;
			info.usage = usage;
			info.size = size;
			info.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
#if defined(LEVK_VKRESOURCE_NAMES)
			info.name = utils::tName<T>();
#endif
			buffer = vram::createBuffer(info);
		}
		return;
	}

	void release()
	{
		vram::release(buffer);
		buffer = Buffer();
		return;
	}

	bool write(T const& data, vk::DescriptorSet set)
	{
		if (!vram::write(buffer, &data))
		{
			return false;
		}
		writer.write(set, buffer, 0);
		return true;
	}
};

class Set final
{
public:
	vk::DescriptorSet m_descriptorSet;

private:
	ShaderBuffer<View> m_view;
	ShaderBuffer<SSBOModels> m_models;
	ShaderBuffer<SSBONormals> m_normals;
	ShaderBuffer<SSBOTints> m_tints;
	ShaderBuffer<SSBOFlags> m_flags;
	ShaderWriter m_diffuse;
	ShaderWriter m_specular;

public:
	void create();
	void destroy();

public:
	void writeView(View const& view);
	void writeSSBO(SSBOModels const& models, SSBONormals const& normals, SSBOTints const& tints, SSBOFlags const& flags);
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
