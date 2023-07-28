#include <spaced/core/logger.hpp>
#include <spaced/resources/material_asset.hpp>
#include <spaced/resources/mesh_asset.hpp>
#include <spaced/resources/primitive_asset.hpp>
#include <spaced/resources/resources.hpp>
#include <spaced/resources/skeleton_asset.hpp>
#include <spaced/vfs/file_reader.hpp>

namespace spaced {
namespace {
auto const g_log{logger::Logger{"MeshAsset"}};
}

auto MeshAsset::try_load(Uri const& uri) -> bool {
	auto const json = read_json(uri);
	if (!json) { return false; }

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
		if (auto const& skeleton_uri = json["skeleton"]) {
			auto const* skeleton_asset = Resources::self().load<SkeletonAsset>(skeleton_uri.as<std::string>());
			if (skeleton_asset == nullptr) { return false; }
			mesh.skeleton = &skeleton_asset->skeleton;
		}
	}
	return true;
}
} // namespace spaced
