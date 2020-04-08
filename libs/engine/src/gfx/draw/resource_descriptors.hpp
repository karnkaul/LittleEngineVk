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
enum class Type : u8
{
	eScene,
	eObject,
	eCOUNT_
};

struct View final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	alignas(16) glm::mat4 mat_vp = glm::mat4(1.0f);
	alignas(16) glm::mat4 mat_v = glm::mat4(1.0f);
	alignas(16) glm::mat4 mat_p = glm::mat4(1.0f);
	alignas(16) glm::mat4 mat_ui = glm::mat4(1.0f);
	alignas(16) glm::vec3 pos_v = glm::vec3(0.0f);
};

struct Locals final
{
	constexpr static u32 max = 1024;

	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	enum
	{
		eTEXTURED = 1 << 0,
		eSPECULAR = 1 << 1,
	};

	alignas(16) glm::mat4 mat_m = glm::mat4(1.0f);
	alignas(16) glm::mat4 mat_n = glm::mat4(1.0f);
	alignas(16) glm::vec4 tint = glm::vec4(1.0f);
	alignas(4) u32 flags = 0;
};

struct Textures final
{
	constexpr static u32 max = 1024;

	static vk::DescriptorSetLayoutBinding const s_diffuseLayoutBinding;
	static vk::DescriptorSetLayoutBinding const s_specularLayoutBinding;
};

struct Push final
{
	u32 localID = 0;
	u32 diffuseID = 0;
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

struct BufferWriter final
{
	Buffer buffer;

	template <typename T>
	bool write(vk::DescriptorSet set, vk::DescriptorType type, T const& data, u32 binding = T::binding, u32 idx = 0)
	{
		u32 size = (u32)sizeof(T);
		if (buffer.writeSize < size)
		{
			vram::release(buffer);
			BufferInfo info;
			info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			info.queueFlags = QFlag::eGraphics;
			info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eUniformBuffer;
			info.size = sizeof(T);
			info.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
#if defined(LEVK_VKRESOURCE_NAMES)
			info.name = utils::tName<T>();
#endif
			buffer = vram::createBuffer(info);
		}
		if (!vram::write(buffer, &data))
		{
			return false;
		}
		writeBuffer(set, type, binding, size, idx);
		return true;
	}

	void writeBuffer(vk::DescriptorSet set, vk::DescriptorType type, u32 binding, u32 size, u32 idx) const;
};

struct TextureWriter
{
	void write(vk::DescriptorSet set, vk::DescriptorType type, Texture const& texture, u32 binding, u32 idx);
};

void write(WriteInfo const& info);

class Sets final
{
private:
	template <typename T>
	struct Handle final
	{
		T writer;
		vk::DescriptorType type;
		u32 binding;

		template <typename U>
		void write(vk::DescriptorSet set, U const& u, u32 idx)
		{
			writer.write(set, type, u, binding, idx);
		}
	};

public:
	std::array<vk::DescriptorSet, (size_t)Type::eCOUNT_> m_sets;

private:
	Handle<BufferWriter> m_view;
	Handle<BufferWriter> m_locals;
	Handle<TextureWriter> m_diffuse;
	Handle<TextureWriter> m_specular;

public:
	Sets();

public:
	void writeView(View const& view);
	void writeLocals(Locals const& flags, u32 idx);
	void writeDiffuse(Texture const& diffuse, u32 idx);
	void writeSpecular(Texture const& specular, u32 idx);

	void resetTextures();

public:
	void destroy();
};

struct SetLayouts final
{
	vk::DescriptorPool descriptorPool;
	std::vector<Sets> sets;
};

inline std::array<vk::DescriptorSetLayout, (size_t)Type::eCOUNT_> g_setLayouts;

void init();
void deinit();

SetLayouts allocateSets(u32 copies);
} // namespace rd
} // namespace le::gfx
