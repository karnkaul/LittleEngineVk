#pragma once
#include <ktl/either.hpp>
#include <levk/graphics/draw_primitive.hpp>
#include <levk/graphics/material_data.hpp>
#include <levk/graphics/mesh_primitive.hpp>
#include <levk/graphics/texture.hpp>
#include <optional>

namespace le {
namespace io {
class Media;
}

namespace graphics {
struct Mesh {
	struct Material {
		TMatTexArray<std::optional<std::size_t>> textures{};
		ktl::either<BPMaterialData, PBRMaterialData> data{};
	};

	std::vector<Sampler> samplers{};
	std::vector<Texture> textures{};
	std::vector<Material> materials{};
	std::vector<MeshPrimitive> primitives{};

	mutable std::vector<MaterialTextures> materialTextures{};

	static std::optional<Mesh> fromObjMtl(io::Path const& jsonURI, io::Media const& media, not_null<VRAM*> vram, vk::Sampler sampler = {});

	Opt<Texture const> texture(std::optional<std::size_t> idx) const noexcept { return idx && *idx < textures.size() ? &textures[*idx] : nullptr; }
	std::vector<DrawPrimitive> primitiveViews() const;
};

template <>
struct DrawPrimitiveAdder<Mesh> {
	template <std::output_iterator<DrawPrimitive> It>
	void operator()(Mesh const& mesh, It it) const {
		mesh.materialTextures.clear();
		mesh.materialTextures.reserve(mesh.primitives.size());
		for (auto const& primitive : mesh.primitives) {
			auto const& mat = mesh.materials[primitive.m_material];
			MaterialTextures matTex;
			for (std::size_t i = 0; i < std::size_t(MatTexType::eCOUNT_); ++i) {
				matTex.arr[i] = mat.textures.arr[i] ? &mesh.textures[*mat.textures.arr[i]] : nullptr;
			}
			mesh.materialTextures.push_back(matTex);
			*it++ = DrawPrimitive{&primitive, &mesh.materialTextures.back(), mat.data.get_if<BPMaterialData>(), mat.data.get_if<PBRMaterialData>()};
		}
	}
};
} // namespace graphics
} // namespace le
