#pragma once
#include <spaced/engine/graphics/resource.hpp>
#include <span>

namespace spaced::graphics {
struct ImageBarrier {
	vk::ImageMemoryBarrier2 barrier{};

	explicit ImageBarrier(vk::Image image, std::uint32_t mip_levels = 1, std::uint32_t array_layers = 1);

	explicit ImageBarrier(Image const& image);

	auto set_full_barrier(vk::ImageLayout src, vk::ImageLayout dst) -> ImageBarrier&;
	auto set_undef_to_optimal(bool depth) -> ImageBarrier&;
	auto set_undef_to_transfer_dst() -> ImageBarrier&;
	auto set_optimal_to_read_only(bool depth) -> ImageBarrier&;
	auto set_optimal_to_transfer_src() -> ImageBarrier&;
	auto set_optimal_to_present() -> ImageBarrier&;
	auto set_transfer_dst_to_optimal(bool depth) -> ImageBarrier&;
	auto set_transfer_dst_to_present() -> ImageBarrier&;

	auto transition(vk::CommandBuffer cmd) const -> void;

	static auto transition(vk::CommandBuffer cmd, std::span<vk::ImageMemoryBarrier2 const> barriers) -> void;
};
} // namespace spaced::graphics
