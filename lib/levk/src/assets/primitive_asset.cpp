#include <levk/asset_store.hpp>
#include <levk/assets/bin_geometry.hpp>
#include <levk/assets/lit_material_asset.hpp>
#include <levk/assets/primitive_asset.hpp>
#include <levk/assets/shader_asset.hpp>
#include <levk/assets/skeleton_asset.hpp>
#include <levk/assets/texture_asset.hpp>
#include <levk/io/json_io.hpp>

namespace levk {
namespace {
struct MeshPrimitive {
	Geometry geometry{};
	vk::PrimitiveTopology topology{};
	Ptr<IMaterial const> material{};
	RenderShader vertex_shader{};
};

constexpr auto get_topology(std::string_view const in) {
	if (in == "line_list") { return vk::PrimitiveTopology::eLineList; }
	if (in == "line_strip") { return vk::PrimitiveTopology::eLineStrip; }
	if (in == "triangle_strip") { return vk::PrimitiveTopology::eTriangleStrip; }
	return Geometry::topology_v;
}

auto load_shader(IAssetStore& store, std::string_view const uri, bool const reload) -> RenderShader {
	auto const material_info = IAsset::LoadInfo{
		.uri = uri,
		.reload = reload,
	};
	auto const* asset = store.load<ShaderAsset>(material_info);
	if (asset == nullptr) { return {}; }
	return asset->get_render_shader();
}

auto load_material(IAssetStore& store, std::string_view const uri, bool const reload) -> Ptr<material::Lit const> {
	auto const material_info = IAsset::LoadInfo{
		.uri = uri,
		.reload = reload,
	};
	auto const* asset = store.load<LitMaterialAsset>(material_info);
	if (asset == nullptr) { return {}; }
	return &*asset->material;
}

auto load_primitive(IAssetStore& store, dj::Json const& json, bool const reload) -> std::optional<MeshPrimitive> {
	auto const shader_uri = json["vertex_shader"].as_string();
	auto const vertex_shader = load_shader(store, shader_uri, reload);
	if (vertex_shader.empty()) { return {}; }

	auto const va_bytes = store.get_vfs().load_bytes(json["vertex_array"].as_string());
	if (va_bytes.empty()) { return {}; }

	auto ret = MeshPrimitive{};

	ret.vertex_shader = vertex_shader;

	from_bytes(va_bytes, ret.geometry.vertex_array);
	if (ret.geometry.vertex_array.is_empty()) { return {}; }

	ret.material = load_material(store, json["material"].as_string(), reload);
	if (ret.material == nullptr) { return {}; }

	if (auto const& in_skin = json["vertex_skin"]) {
		auto const skin_bytes = store.get_vfs().load_bytes(in_skin.as_string());
		if (skin_bytes.empty()) { return {}; }
		from_bytes(skin_bytes, ret.geometry.vertex_skin);
	}

	ret.geometry.topology = get_topology(json["topology"].as_string());

	return ret;
}
} // namespace

auto StaticPrimitiveAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	auto const json = store.get_vfs().load_json(info.uri);
	if (IAssetStore::read_asset_type_name(json) != type_name_v) { return false; }

	auto loaded = load_primitive(store, json, info.reload);
	if (!loaded) { return false; }

	primitive = store.get_engine().get_render_device().create_static_primitive(loaded->geometry, loaded->material, loaded->vertex_shader);
	return true;
}

auto SkinnedPrimitiveAsset::load(IAssetStore& store, LoadInfo const& info) -> bool {
	auto const json = store.get_vfs().load_json(info.uri);
	if (IAssetStore::read_asset_type_name(json) != type_name_v) { return false; }

	auto loaded = load_primitive(store, json, info.reload);
	if (!loaded) { return false; }

	primitive = store.get_engine().get_render_device().create_skinned_primitive(loaded->geometry, loaded->material, loaded->vertex_shader);
	return true;
}
} // namespace levk
