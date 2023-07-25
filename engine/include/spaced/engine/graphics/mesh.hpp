#pragma once
#include <spaced/engine/core/not_null.hpp>
#include <spaced/engine/graphics/material.hpp>
#include <spaced/engine/graphics/primitive.hpp>

namespace spaced::graphics {
struct MeshPrimitive {
	NotNull<Primitive const*> primitive;
	Ptr<Material const> material{};
};

struct Mesh {
	std::vector<MeshPrimitive> primitives{};
};
} // namespace spaced::graphics
