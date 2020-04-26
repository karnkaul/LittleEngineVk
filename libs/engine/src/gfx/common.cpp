#include "engine/gfx/mesh.hpp"
#include "common.hpp"

namespace le::gfx
{
std::unordered_map<vk::Result, std::string_view> g_vkResultStr = {
	{vk::Result::eErrorOutOfHostMemory, "OutOfHostMemory"},
	{vk::Result::eErrorOutOfDeviceMemory, "OutOfDeviceMemory"},
	{vk::Result::eSuccess, "Success"},
	{vk::Result::eSuboptimalKHR, "SubmoptimalSurface"},
	{vk::Result::eErrorDeviceLost, "DeviceLost"},
	{vk::Result::eErrorSurfaceLostKHR, "SurfaceLost"},
	{vk::Result::eErrorFullScreenExclusiveModeLostEXT, "FullScreenExclusiveModeLost"},
	{vk::Result::eErrorOutOfDateKHR, "OutOfDateSurface"},
};

ScreenRect::ScreenRect(glm::vec4 const& ltrb) noexcept : left(ltrb.x), top(ltrb.y), right(ltrb.z), bottom(ltrb.w) {}

ScreenRect::ScreenRect(glm::vec2 const& size, glm::vec2 const& leftTop) noexcept
	: left(leftTop.x), top(leftTop.y), right(leftTop.x + size.x), bottom(leftTop.y + size.y)
{
}

f32 ScreenRect::aspect() const
{
	glm::vec2 const size = {right - left, bottom - top};
	return size.x / size.y;
}

vk::ShaderModule ShaderImpl::module(Shader::Type type) const
{
	ASSERT(shaders.at((size_t)type) != vk::ShaderModule(), "Module not present in Shader!");
	return shaders.at((size_t)type);
}

std::array<vk::ShaderStageFlagBits, size_t(Shader::Type::eCOUNT_)> const ShaderImpl::s_typeToFlagBit = {vk::ShaderStageFlagBits::eVertex,
																										vk::ShaderStageFlagBits::eFragment};

std::map<Shader::Type, vk::ShaderModule> ShaderImpl::modules() const
{
	std::map<Shader::Type, vk::ShaderModule> ret;
	for (size_t idx = 0; idx < (size_t)Shader::Type::eCOUNT_; ++idx)
	{
		auto const& module = shaders.at(idx);
		if (module != vk::ShaderModule())
		{
			ret[(Shader::Type)idx] = module;
		}
	}
	return ret;
}

vk::VertexInputBindingDescription vbo::bindingDescription()
{
	vk::VertexInputBindingDescription ret;
	ret.binding = vertexBinding;
	ret.stride = sizeof(Vertex);
	ret.inputRate = vk::VertexInputRate::eVertex;
	return ret;
}

std::vector<vk::VertexInputAttributeDescription> vbo::attributeDescriptions()
{
	std::vector<vk::VertexInputAttributeDescription> ret;
	vk::VertexInputAttributeDescription pos;
	pos.binding = vertexBinding;
	pos.location = 0;
	pos.format = vk::Format::eR32G32B32Sfloat;
	pos.offset = offsetof(Vertex, position);
	ret.push_back(pos);
	vk::VertexInputAttributeDescription col;
	col.binding = vertexBinding;
	col.location = 1;
	col.format = vk::Format::eR32G32B32Sfloat;
	col.offset = offsetof(Vertex, colour);
	ret.push_back(col);
	vk::VertexInputAttributeDescription norm;
	norm.binding = vertexBinding;
	norm.location = 2;
	norm.format = vk::Format::eR32G32B32Sfloat;
	norm.offset = offsetof(Vertex, normal);
	ret.push_back(norm);
	vk::VertexInputAttributeDescription uv;
	uv.binding = vertexBinding;
	uv.location = 3;
	uv.format = vk::Format::eR32G32Sfloat;
	uv.offset = offsetof(Vertex, texCoord);
	ret.push_back(uv);
	return ret;
}
} // namespace le::gfx
