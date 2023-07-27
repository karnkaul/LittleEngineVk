#pragma once
#include <spaced/engine/graphics/animation/animation.hpp>
#include <spaced/engine/resources/asset.hpp>

namespace spaced {
class AnimationAsset : public Asset {
  public:
	static constexpr std::string_view type_name_v{"AnimationAsset"};

	[[nodiscard]] auto type_name() const -> std::string_view final { return type_name_v; }
	[[nodiscard]] auto try_load(Uri const& uri) -> bool final;

	static auto bin_pack_to(std::vector<std::uint8_t>& out, graphics::Animation::Channel const& sampler) -> void;
	static auto bin_unpack_from(std::span<std::uint8_t const> bytes, graphics::Animation::Channel& out) -> bool;
	static auto bin_pack_to(std::vector<std::uint8_t>& out, graphics::Animation const& animation) -> void;
	static auto bin_unpack_from(std::span<std::uint8_t const> bytes, graphics::Animation& out) -> bool;

	graphics::Animation animation{};
};
} // namespace spaced
