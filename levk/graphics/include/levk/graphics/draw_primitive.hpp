#pragma once
#include <levk/graphics/material_data.hpp>
#include <iterator>

namespace le::graphics {
class MeshPrimitive;
class Texture;

using MaterialTextures = TMatTexArray<Opt<Texture const>>;

struct DrawPrimitive {
	MaterialTextures textures{};
	Opt<MeshPrimitive const> primitive{};
	Opt<BPMaterialData const> blinnPhong{};
	Opt<PBRMaterialData const> pbr{};

	explicit operator bool() const noexcept { return primitive && (blinnPhong || pbr); }
};

template <typename T>
struct AddDrawPrimitives;

template <typename T>
concept DrawPrimitiveAPI = requires(T t, DrawPrimitive& p) {
	AddDrawPrimitives<T>{}(t, &p);
};
} // namespace le::graphics
