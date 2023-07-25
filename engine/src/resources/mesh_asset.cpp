#include <spaced/engine/core/logger.hpp>
#include <spaced/engine/resources/bin_data.hpp>
#include <spaced/engine/resources/material_asset.hpp>
#include <spaced/engine/resources/mesh_asset.hpp>
#include <spaced/engine/resources/primitive_asset.hpp>
#include <spaced/engine/resources/resources.hpp>
#include <spaced/engine/vfs/file_reader.hpp>

namespace spaced {
static_assert(bin::TrivialT<graphics::Vertex>);

auto MeshAsset::try_load(Uri const& uri) -> bool {
	auto const json = read_json(uri);
	if (!json) { return false; }

	auto const mesh_type = json["mesh_type"].as_string();
	if (mesh_type != "static") {
		logger::g_log.warn("mesh type '{}' is currently unsupported", mesh_type);
		return false;
	}

	mesh = {};
	for (auto const& in_primitive : json["primitives"].array_view()) {
		auto const geometry_uri = in_primitive["geometry"].as<std::string>();
		if (geometry_uri.empty()) { continue; }
		auto const* primitive_asset = Resources::self().load<PrimitiveAsset>(geometry_uri);
		if (primitive_asset == nullptr) { return false; }
		auto const material_uri = in_primitive["material"].as<std::string>();
		auto const* material_asset = [&] {
			if (material_uri.empty()) { return Ptr<MaterialAsset>{}; }
			return Resources::self().load<MaterialAsset>(material_uri);
		}();
		auto const* material = material_asset != nullptr ? material_asset->material.get() : nullptr;
		mesh.primitives.push_back(graphics::MeshPrimitive{&primitive_asset->primitive, material});
	}
	return true;
}
} // namespace spaced
