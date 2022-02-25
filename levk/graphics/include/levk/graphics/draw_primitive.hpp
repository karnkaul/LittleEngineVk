#pragma once
#include <levk/graphics/material_data.hpp>
#include <iterator>

namespace le::graphics {
class MeshPrimitive;
class Texture;

using MaterialTextures = TMatTexArray<Opt<Texture const>>;

struct DrawPrimitive {
	Opt<MeshPrimitive const> primitive{};
	Opt<MaterialTextures const> textures{};
	Opt<BPMaterialData const> blinnPhong{};
	Opt<PBRMaterialData const> pbr{};

	explicit operator bool() const noexcept { return primitive && textures && (blinnPhong || pbr); }
	Opt<Texture const> texture(MatTexType type) const noexcept { return textures ? (*textures)[type] : nullptr; }
};

template <typename T>
struct DrawPrimitiveAdder {
	template <std::output_iterator<DrawPrimitive> It>
	void operator()(T const& t, It it) const;
};
} // namespace le::graphics
