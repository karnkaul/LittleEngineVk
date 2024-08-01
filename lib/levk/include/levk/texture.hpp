#pragma once
#include <levk/bitmap.hpp>
#include <levk/core/polymorphic.hpp>
#include <levk/descriptor_info.hpp>
#include <string>

namespace levk {
/// \brief Texture sampler.
struct TextureSampler {
	vk::SamplerAddressMode wrap_u{vk::SamplerAddressMode::eClampToEdge};
	vk::SamplerAddressMode wrap_v{vk::SamplerAddressMode::eClampToEdge};
	vk::SamplerAddressMode wrap_w{vk::SamplerAddressMode::eClampToEdge};
	vk::Filter min_filter{vk::Filter::eLinear};
	vk::Filter mag_filter{vk::Filter::eLinear};
	vk::BorderColor border{vk::BorderColor::eIntOpaqueBlack};
	float anisotropy{8.0f};

	auto operator==(TextureSampler const&) const -> bool = default;
};

/// \brief Opaque interface for Textures: images that can be sampled in render passes.
class ITexture : public Polymorphic {
  public:
	using Sampler = TextureSampler;

	[[nodiscard]] virtual auto get_size() const -> glm::ivec2 = 0;
	[[nodiscard]] virtual auto get_descriptor_info(std::uint32_t binding) const -> DescriptorInfo = 0;

	Sampler sampler{};

	std::string name{};
};

class IDynamicTexture : public ITexture {
  public:
	virtual void recreate(BitmapView bitmap) = 0;
};

class IRenderTexture : public ITexture {};

class ICubemap : public ITexture {};
} // namespace levk
