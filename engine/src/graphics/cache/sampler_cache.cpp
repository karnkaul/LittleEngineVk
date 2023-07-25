#include <spaced/engine/core/hash_combine.hpp>
#include <spaced/engine/graphics/cache/sampler_cache.hpp>
#include <spaced/engine/graphics/device.hpp>

namespace spaced::graphics {
namespace {
constexpr auto from(TextureSampler::Wrap const wrap) -> vk::SamplerAddressMode {
	switch (wrap) {
	case TextureSampler::Wrap::eClampBorder: return vk::SamplerAddressMode::eClampToBorder;
	case TextureSampler::Wrap::eClampEdge: return vk::SamplerAddressMode::eClampToEdge;
	default: return vk::SamplerAddressMode::eRepeat;
	}
}

constexpr auto from(TextureSampler::Filter const filter) -> vk::Filter {
	switch (filter) {
	case TextureSampler::Filter::eNearest: return vk::Filter::eNearest;
	default: return vk::Filter::eLinear;
	}
}

constexpr auto from(TextureSampler::Border const border) -> vk::BorderColor {
	switch (border) {
	default:
	case TextureSampler::Border::eOpaqueBlack: return vk::BorderColor::eFloatOpaqueBlack;
	case TextureSampler::Border::eOpaqueWhite: return vk::BorderColor::eFloatOpaqueWhite;
	case TextureSampler::Border::eTransparentBlack: return vk::BorderColor::eFloatTransparentBlack;
	}
}
} // namespace

auto SamplerCache::Hasher::operator()(TextureSampler const& sampler) const -> std::size_t {
	return make_combined_hash(sampler.min, sampler.mag, sampler.wrap_s, sampler.wrap_t, sampler.border);
}

auto SamplerCache::get(TextureSampler const& sampler) -> vk::Sampler {
	if (auto it = map.find(sampler); it != map.end()) { return *it->second; }
	auto sci = vk::SamplerCreateInfo{};
	sci.minFilter = from(sampler.min);
	sci.magFilter = from(sampler.mag);
	sci.anisotropyEnable = anisotropy > 0.0f ? 1 : 0;
	sci.maxAnisotropy = anisotropy;
	sci.borderColor = from(sampler.border);
	sci.mipmapMode = vk::SamplerMipmapMode::eNearest;
	sci.addressModeU = from(sampler.wrap_s);
	sci.addressModeV = from(sampler.wrap_t);
	sci.addressModeW = from(sampler.wrap_s);
	sci.maxLod = VK_LOD_CLAMP_NONE;
	auto [it, _] = map.insert_or_assign(sampler, Device::self().device().createSamplerUnique(sci));
	return *it->second;
}
} // namespace spaced::graphics
