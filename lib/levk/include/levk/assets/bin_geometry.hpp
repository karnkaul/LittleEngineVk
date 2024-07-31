#pragma once
#include <levk/geometry.hpp>
#include <cstddef>
#include <span>
#include <vector>

namespace levk {
void to_bytes(std::vector<std::byte>& out, VertexArray const& vertex_array);
void from_bytes(std::span<std::byte const> bytes, VertexArray& out);

void to_bytes(std::vector<std::byte>& out, VertexSkin const& vertex_skin);
void from_bytes(std::span<std::byte const> bytes, VertexSkin& out);
} // namespace levk
