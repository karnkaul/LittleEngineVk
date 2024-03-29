#include <le/resources/material_asset.hpp>
#include <le/resources/mesh_asset.hpp>
#include <le/resources/primitive_asset.hpp>
#include <le/resources/resources.hpp>
#include <le/resources/skeleton_asset.hpp>
#include <le/vfs/file_reader.hpp>

namespace le {
auto MeshAsset::try_load(Uri const& uri) -> bool {
	auto const json = read_json(uri);
	if (!json) { return false; }

	mesh = {};
	for (auto const& in_primitive : json["primitives"].array_view()) {
		auto const geometry_uri = in_primitive["geometry"].as_string();
		if (geometry_uri.empty()) { continue; }
		auto const* primitive_asset = Resources::self().load<PrimitiveAsset>(geometry_uri);
		if (primitive_asset == nullptr) { return false; }
		auto const material_uri = in_primitive["material"].as_string();
		auto const* material_asset = [&] {
			if (material_uri.empty()) { return Ptr<MaterialAsset>{}; }
			return Resources::self().load<MaterialAsset>(material_uri);
		}();
		auto const* material = material_asset != nullptr ? material_asset->material.get() : nullptr;
		mesh.primitives.push_back(graphics::MeshPrimitive{&primitive_asset->primitive, material});
		if (auto const& skeleton_uri = json["skeleton"]) {
			auto const* skeleton_asset = Resources::self().load<SkeletonAsset>(skeleton_uri.as_string());
			if (skeleton_asset == nullptr) { return false; }
			mesh.skeleton = &skeleton_asset->skeleton;
		}
	}
	return true;
}
} // namespace le
