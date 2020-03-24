#pragma once
#include <array>
#include "gfx/vram.hpp"

namespace le::gfx::ubo
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
	}

	static Handle<T> create(vk::DescriptorSetLayout setLayout, vk::DescriptorSet descriptorSet)
	{
		Handle<T> ret;
		ret.setLayout = setLayout;
		ret.descriptorSet = descriptorSet;
		BufferData data;
		data.properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		data.usage = vk::BufferUsageFlagBits::eUniformBuffer;
		data.size = ret.size;
		data.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		ret.buffer = vram::createBuffer(data);
		writeUniformDescriptor(ret.buffer, ret.descriptorSet, T::binding);
		return ret;
	}
};

struct View final
{
	static constexpr u32 binding = 0;

	glm::mat4 mat_vp = glm::mat4(1.0f);
	glm::mat4 mat_v = glm::mat4(1.0f);
};

struct UBOs final
{
	Handle<View> view;
};
} // namespace le::gfx::ubo
