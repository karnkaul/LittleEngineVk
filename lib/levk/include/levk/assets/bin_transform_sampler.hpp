#pragma once
#include <levk/transform_sampler.hpp>
#include <cstddef>
#include <span>
#include <vector>

namespace levk {
void to_bytes(std::vector<std::byte>& out, TransformSampler const& transform_sampler);
void from_bytes(std::span<std::byte const> bytes, TransformSampler& out);

void to_bytes(std::vector<std::byte>& out, std::span<TransformSampler const> samplers);
void from_bytes(std::span<std::byte const> bytes, std::vector<TransformSampler>& out);
} // namespace levk
