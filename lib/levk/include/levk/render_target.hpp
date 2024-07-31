#pragma once
#include <levk/descriptor_info.hpp>
#include <levk/render_resource.hpp>

namespace levk {
/// \brief Render Targets for rendering.
struct RenderTarget {
	struct Image {
		vk::Image image{};
		vk::ImageView view{};
		vk::Format format{};
	};

	Image colour{};
	Image depth{};
	Image resolve{};
	vk::Extent2D extent{};

	[[nodiscard]] auto is_empty() const -> bool { return !colour.image && !depth.image; }

	[[nodiscard]] auto get_output() const -> Image {
		if (resolve.image) { return resolve; }
		if (colour.image) { return colour; };
		if (depth.image) { return depth; }
		return {};
	}

	[[nodiscard]] auto get_descriptor_info(vk::Sampler const sampler, std::uint32_t const binding) const -> DescriptorInfo {
		auto const image = get_output();
		if (!image.image) { return {}; }

		return DescriptorInfo{
			.payload = vk::DescriptorImageInfo{sampler, image.view, vk::ImageLayout::eShaderReadOnlyOptimal},
			.type = vk::DescriptorType::eCombinedImageSampler,
			.binding = binding,
		};
	}
};
} // namespace levk
