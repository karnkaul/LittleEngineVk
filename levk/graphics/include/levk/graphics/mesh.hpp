#pragma once
#include <ktl/either.hpp>
#include <levk/graphics/material_data.hpp>
#include <levk/graphics/mesh_primitive.hpp>
#include <levk/graphics/mesh_view.hpp>
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

	static std::optional<Mesh> fromObjMtl(io::Path const& jsonURI, io::Media const& media, not_null<VRAM*> vram, vk::Sampler sampler = {});

	Opt<Texture const> texture(std::optional<std::size_t> idx) const noexcept { return idx && *idx < textures.size() ? &textures[*idx] : nullptr; }
	MeshView view() const;
};
} // namespace graphics
} // namespace le
