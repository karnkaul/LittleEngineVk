#include "vertex.hpp"

namespace le::vuk
{
vk::VertexInputBindingDescription Vertex::bindingDescription(u32 binding)
{
	vk::VertexInputBindingDescription ret;
	ret.binding = binding;
	ret.stride = sizeof(Vertex);
	ret.inputRate = vk::VertexInputRate::eVertex;
	return ret;
}

std::vector<vk::VertexInputAttributeDescription> Vertex::attributeDescriptions(u32 binding)
{
	std::vector<vk::VertexInputAttributeDescription> ret;
	vk::VertexInputAttributeDescription pos;
	pos.binding = binding;
	pos.location = 0;
	pos.format = vk::Format::eR32G32Sfloat;
	pos.format = vk::Format(VK_FORMAT_R32G32_SFLOAT);
	pos.offset = offsetof(Vertex, position);
	ret.push_back(pos);
	vk::VertexInputAttributeDescription col;
	col.binding = binding;
	col.location = 1;
	col.format = vk::Format::eR32G32B32Sfloat;
	col.format = vk::Format(VK_FORMAT_R32G32B32_SFLOAT);
	col.offset = offsetof(Vertex, colour);
	ret.push_back(col);
	return ret;
}
} // namespace le::vuk
