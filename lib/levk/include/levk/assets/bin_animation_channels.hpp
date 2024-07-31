#pragma once
#include <levk/tree_animation.hpp>
#include <cstddef>
#include <span>
#include <vector>

namespace levk {
void to_bytes(std::vector<std::byte>& out, std::span<AnimationChannel const> animation_channels);
void from_bytes(std::span<std::byte const> bytes, std::vector<AnimationChannel>& out);
} // namespace levk
