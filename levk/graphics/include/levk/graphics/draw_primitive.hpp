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
struct AddDrawPrimitives {
	template <std::output_iterator<DrawPrimitive> It>
	void operator()(T const& t, It it) const;
};
} // namespace le::graphics
