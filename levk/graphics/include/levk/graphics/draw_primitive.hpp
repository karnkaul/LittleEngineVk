#pragma once
#include <levk/graphics/material_data.hpp>
#include <concepts>

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
concept DrawPrimitiveAPI = requires(T t, DrawPrimitive* it) {
	AddDrawPrimitives<T>{}(t, it);
};

template <typename T>
concept GetDrawPrimitive = requires(T t) {
	{ t.drawPrimitive() } -> std::convertible_to<DrawPrimitive>;
};

template <typename T>
	requires GetDrawPrimitive<T>
struct AddDrawPrimitives<T> {
	template <std::output_iterator<DrawPrimitive> It>
	void operator()(T const& t, It it) const {
		*it++ = t.drawPrimitive();
	}
};
} // namespace le::graphics
