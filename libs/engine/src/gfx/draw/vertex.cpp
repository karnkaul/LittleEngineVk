#include "vertex.hpp"

namespace le::gfx
{
vk::VertexInputBindingDescription Vertex::bindingDescription()
{
	vk::VertexInputBindingDescription ret;
	ret.binding = binding;
	ret.stride = sizeof(Vertex);
	ret.inputRate = vk::VertexInputRate::eVertex;
	return ret;
}

std::vector<vk::VertexInputAttributeDescription> Vertex::attributeDescriptions()
{
	std::vector<vk::VertexInputAttributeDescription> ret;
	vk::VertexInputAttributeDescription pos;
	pos.binding = binding;
	pos.location = 0;
	pos.format = vk::Format::eR32G32B32Sfloat;
	pos.offset = offsetof(Vertex, position);
	ret.push_back(pos);
	vk::VertexInputAttributeDescription col;
	col.binding = binding;
	col.location = 1;
	col.format = vk::Format::eR32G32B32Sfloat;
	col.offset = offsetof(Vertex, colour);
	ret.push_back(col);
	vk::VertexInputAttributeDescription uv;
	uv.binding = binding;
	uv.location = 2;
	uv.format = vk::Format::eR32G32Sfloat;
	uv.offset = offsetof(Vertex, texCoord);
	ret.push_back(uv);
	return ret;
}
} // namespace le::gfx
