#pragma once
#include <levk/core/not_null.hpp>
#include <levk/graphics/material_data.hpp>
#include <vector>

namespace le::graphics {
class MeshPrimitive;
class Texture;

struct PrimitiveView {
	TMatTexArray<Opt<Texture const>> textures{};
	not_null<MeshPrimitive const*> primitive;
	Opt<BPMaterialData const> blinnPhong{};
	Opt<PBRMaterialData const> pbr{};

	static PrimitiveView make(not_null<MeshPrimitive const*> primitive, Opt<BPMaterialData const> material) { return {{}, primitive, material}; }
	static PrimitiveView make(not_null<MeshPrimitive const*> primitive, Opt<PBRMaterialData const> material) { return {{}, primitive, {}, material}; }
};

struct MeshView {
	std::vector<PrimitiveView> primitives;

	explicit operator bool() const noexcept { return !primitives.empty(); }
};
} // namespace le::graphics
