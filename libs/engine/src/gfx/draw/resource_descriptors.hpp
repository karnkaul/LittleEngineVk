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
	bool write(vk::DescriptorSet set, T const& data, u32 binding = T::binding)
	{
		u32 size = (u32)sizeof(T);
		if (buffer.writeSize < size)
		{
			vram::release(buffer);
			BufferInfo info;
			info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			info.queueFlags = QFlag::eGraphics;
			info.usage = vk::BufferUsageFlagBits::eUniformBuffer;
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
		writeBuffer(set, binding, size);
		return true;
	}

	void writeBuffer(vk::DescriptorSet set, u32 binding, u32 size) const;
};

struct TextureWriter
{
	void write(vk::DescriptorSet set, Texture const& texture, u32 binding);
};

void write(WriteInfo const& info);

struct View final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	alignas(16) glm::mat4 mat_vp = glm::mat4(1.0f);
	alignas(16) glm::mat4 mat_v = glm::mat4(1.0f);
};

struct Flags final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	enum
	{
		eTEXTURED = 1 << 0,
	};

	alignas(16) glm::vec4 tint = glm::vec4(1.0f);
	alignas(4) u32 bits = 0;
};

struct Textures final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;
};

class Sets final
{
private:
	template <typename T>
	struct Handle final
	{
		T writer;
		u32 binding;

		template <typename U>
		void write(vk::DescriptorSet set, U const& u)
		{
			writer.write(set, u, binding);
		}
	};

public:
	std::array<vk::DescriptorSet, (size_t)Type::eCOUNT_> m_sets;

private:
	Handle<BufferWriter> m_view;
	Handle<BufferWriter> m_flags;
	Handle<TextureWriter> m_diffuse;

public:
	Sets();

public:
	void writeView(View const& view);
	void writeFlags(Flags const& flags);
	void writeDiffuse(Texture const& diffuse);

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
