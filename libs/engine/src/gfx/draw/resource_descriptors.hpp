#pragma once
#include <vector>
#include "gfx/common.hpp"
#include "gfx/vram.hpp"

namespace le::gfx
{
class Texture;

namespace rd
{
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
} // namespace rd

namespace ubo
{
struct View final
{
	static constexpr u32 binding = 0;

	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	glm::mat4 mat_vp = glm::mat4(1.0f);
	glm::mat4 mat_v = glm::mat4(1.0f);
};

struct Flags final
{
	static constexpr u32 binding = 1;

	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	s32 isTextured = 0;
};
} // namespace ubo

namespace rd
{
enum class Type : u8
{
	eUniformBuffer,
	eCOUNT_
};

class Set final
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
	vk::DescriptorSet m_set;

private:
	Handle<BufferWriter> m_view = {{}, 0};
	Handle<BufferWriter> m_flags = {{}, 1};
	Handle<TextureWriter> m_diffuse = {{}, 2};

public:
	void writeView(ubo::View const& view);
	void writeFlags(ubo::Flags const& flags);
	void writeDiffuse(Texture const& diffuse);

public:
	void destroy();
};

struct Setup final
{
	vk::DescriptorPool descriptorPool;
	std::vector<Set> sets;
};

struct SetLayouts final
{
	std::array<vk::DescriptorSetLayout, (size_t)Type::eCOUNT_> layouts;
	u32 descriptorCount = 0;

	Setup allocateSets(u32 descriptorSetCount);

	static SetLayouts create();
};

inline SetLayouts g_setLayouts;

void init();
void deinit();
} // namespace rd
} // namespace le::gfx
