#include <glm/mat4x4.hpp>
#include <spaced/graphics/geometry.hpp>
#include <spaced/graphics/shader_layout.hpp>

namespace spaced::graphics {
auto VertexLayout::make(Buffers const& buffers) -> VertexLayout {
	auto ret = VertexLayout{.buffers = buffers};

	auto const& gbo = buffers.geometry;
	auto const& sbo = buffers.skeleton;

	ret.attributes = {
		vk::VertexInputAttributeDescription{gbo.location + 0, gbo.buffer, vk::Format::eR32G32B32Sfloat, offsetof(graphics::Vertex, position)},
		vk::VertexInputAttributeDescription{gbo.location + 1, gbo.buffer, vk::Format::eR32G32B32A32Sfloat, offsetof(graphics::Vertex, rgba)},
		vk::VertexInputAttributeDescription{gbo.location + 2, gbo.buffer, vk::Format::eR32G32B32Sfloat, offsetof(graphics::Vertex, normal)},
		vk::VertexInputAttributeDescription{gbo.location + 3, gbo.buffer, vk::Format::eR32G32Sfloat, offsetof(graphics::Vertex, uv)},

		vk::VertexInputAttributeDescription{sbo.location, sbo.buffer, vk::Format::eR32G32B32A32Uint, offsetof(graphics::Bone, joint)},
		vk::VertexInputAttributeDescription{sbo.location + 1, sbo.buffer, vk::Format::eR32G32B32A32Sfloat, offsetof(graphics::Bone, weight)},
	};

	ret.bindings = {
		vk::VertexInputBindingDescription{gbo.buffer, sizeof(graphics::Vertex)},

		vk::VertexInputBindingDescription{sbo.buffer, sizeof(graphics::Bone)},
	};

	return ret;
}
} // namespace spaced::graphics
