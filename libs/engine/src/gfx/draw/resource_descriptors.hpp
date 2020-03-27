#pragma once
#include "gfx/common.hpp"
#include "gfx/vram.hpp"

namespace le::gfx
{
namespace ubo
{
template <typename T>
struct Handle final
{
	static constexpr vk::DeviceSize size = sizeof(T);

	Buffer buffer;
	vk::DescriptorSetLayout setLayout;
	vk::DescriptorSet descriptorSet;
	vk::DeviceSize offset;

	void write(T const& data) const
	{
		vram::write(buffer, &data);
		return;
	}

	static Handle<T> create(vk::DescriptorSetLayout setLayout, vk::DescriptorSet descriptorSet)
	{
		Handle<T> ret;
		ret.setLayout = setLayout;
		ret.descriptorSet = descriptorSet;
		BufferInfo info;
		info.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		info.queueFlags = QFlag::eGraphics;
		info.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		info.size = ret.size;
		info.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		ret.buffer = vram::createBuffer(info);
		vk::DescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = ret.buffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = ret.buffer.writeSize;
		vk::WriteDescriptorSet descWrite;
		descWrite.dstSet = ret.descriptorSet;
		descWrite.dstBinding = T::binding;
		descWrite.dstArrayElement = 0;
		descWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
		descWrite.descriptorCount = 1;
		descWrite.pBufferInfo = &bufferInfo;
		g_info.device.updateDescriptorSets(descWrite, {});
		return ret;
	}
};

struct View final
{
	static constexpr u32 binding = 0;

	glm::mat4 mat_vp = glm::mat4(1.0f);
	glm::mat4 mat_v = glm::mat4(1.0f);
};

struct Flags final
{
	static constexpr u32 binding = 1;

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

struct Handles final
{
	ubo::Handle<ubo::View> view;
	ubo::Handle<ubo::Flags> flags;

	void release();
};

struct Setup final
{
	vk::DescriptorPool pool;
	std::vector<Handles> descriptorHandles;
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
