#pragma once
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <map>
#include <vector>

namespace spaced::graphics {
struct VertexBinding {
	std::uint32_t buffer{};
	std::uint32_t location{};
};

struct VertexLayout {
	struct Buffers {
		VertexBinding geometry{0, 0};
		VertexBinding skeleton{1, 4};
	};

	std::vector<vk::VertexInputAttributeDescription> attributes{};
	std::vector<vk::VertexInputBindingDescription> bindings{};
	Buffers buffers{};

	static auto make(Buffers const& buffers) -> VertexLayout;
};

struct ShaderLayout {
	using Type = vk::DescriptorType;

	template <Type>
	using Binding = std::uint32_t;

	struct Camera {
		std::uint32_t set{0};
		Binding<Type::eUniformBuffer> view{0};
		Binding<Type::eStorageBuffer> directional_lights{1};
	};

	struct Material {
		std::uint32_t set{1};
		Binding<Type::eUniformBuffer> data{0};
		std::array<Binding<Type::eCombinedImageSampler>, 3> textures{1, 2, 3};
	};

	struct Object {
		std::uint32_t set{2};
		Binding<Type::eStorageBuffer> instances{0};
		Binding<Type::eStorageBuffer> joints{1};
	};

	VertexLayout vertex_layout{VertexLayout::make(VertexLayout::Buffers{})};
	Camera camera{};
	Material material{};
	Object object{};
	std::map<std::uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> custom_sets{};
};
} // namespace spaced::graphics
