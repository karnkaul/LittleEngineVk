#pragma once
#include <vulkan/vulkan.hpp>
#include "core/std_types.hpp"

namespace le::gfx
{
struct MeshImpl final
{
	using ID = TZero<u64, 0>;

	gfx::Buffer vbo;
	gfx::Buffer ibo;
	vk::Fence vboCopied;
	vk::Fence iboCopied;
	ID meshID;
	u32 vertexCount = 0;
	u32 indexCount = 0;
};

namespace vbo
{
constexpr u32 vertexBinding = 0;

vk::VertexInputBindingDescription bindingDescription();
std::vector<vk::VertexInputAttributeDescription> attributeDescriptions();
} // namespace vbo
} // namespace le::gfx
