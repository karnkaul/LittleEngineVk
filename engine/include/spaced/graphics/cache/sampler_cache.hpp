#pragma once
#include <spaced/core/mono_instance.hpp>
#include <spaced/graphics/texture_sampler.hpp>
#include <vulkan/vulkan.hpp>
#include <unordered_map>

namespace spaced::graphics {
class SamplerCache : public MonoInstance<SamplerCache> {
  public:
	auto get(TextureSampler const& sampler) -> vk::Sampler;

	float anisotropy{};

  private:
	struct Hasher {
		auto operator()(TextureSampler const& sampler) const -> std::size_t;
	};

	std::unordered_map<TextureSampler, vk::UniqueSampler, Hasher> map{};
};
} // namespace spaced::graphics
