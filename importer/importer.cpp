#include <clap/clap.hpp>
#include <gltf2cpp/gltf2cpp.hpp>
#include <spaced/engine/core/enumerate.hpp>
#include <spaced/engine/core/zip_ranges.hpp>
#include <spaced/engine/error.hpp>
#include <spaced/engine/resources/bin_data.hpp>
#include <spaced/engine/resources/material_asset.hpp>
#include <spaced/engine/resources/mesh_asset.hpp>
#include <spaced/engine/resources/primitive_asset.hpp>
#include <spaced/engine/resources/texture_asset.hpp>
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>

namespace spaced::importer {
namespace {
namespace fs = std::filesystem;

struct MeshList {
	std::vector<std::size_t> static_meshes{};
	std::vector<std::size_t> skinned_meshes{};

	static auto build(gltf2cpp::Root const& root) -> MeshList {
		auto ret = MeshList{};
		for (auto const [mesh, index] : enumerate(root.meshes)) {
			if (mesh.primitives.empty()) { continue; }
			if (mesh.primitives.front().geometry.joints.empty()) {
				ret.static_meshes.push_back(index);
			} else {
				ret.skinned_meshes.push_back(index);
			}
		}
		return ret;
	}

	[[nodiscard]] auto is_static(std::size_t const index) const { return std::ranges::find(static_meshes, index) != static_meshes.end(); }
	[[nodiscard]] auto is_skinned(std::size_t const index) const { return std::ranges::find(skinned_meshes, index) != skinned_meshes.end(); }
};

template <glm::length_t Dim, typename T>
constexpr auto to_glm_vec(std::span<T const, Dim> in) {
	auto ret = glm::vec<static_cast<glm::length_t>(Dim), T>{};
	ret.x = in[0];
	if constexpr (Dim > 1) { ret.y = Dim > 1 ? in[1] : T{}; }
	if constexpr (Dim > 2) { ret.z = Dim > 2 ? in[2] : T{}; }
	if constexpr (Dim > 3) { ret.w = Dim > 3 ? in[3] : T{}; }
	return ret;
}

[[nodiscard]] auto to_geometry(gltf2cpp::Geometry const& in) -> graphics::Geometry {
	auto ret = graphics::Geometry{};
	auto const has_colour = !in.colors.empty() && in.colors.front().size() == in.positions.size();
	auto const has_uv = !in.tex_coords.empty() && in.tex_coords.front().size() == in.positions.size();
	auto const has_normal = in.normals.size() == in.positions.size();
	for (std::size_t i = 0; i < in.positions.size(); ++i) {
		auto vertex = graphics::Vertex{.position = to_glm_vec<3, float>(in.positions[i])};
		if (has_colour) { vertex.rgba = glm::vec4{to_glm_vec<3, float>(in.colors[0][i]), 1.0f}; }
		if (has_uv) { vertex.uv = to_glm_vec<2, float>(in.tex_coords[0][i]); }
		if (has_normal) { vertex.normal = to_glm_vec<3, float>(in.normals[i]); }
		ret.vertices.push_back(vertex);
	}
	ret.indices = in.indices;
	if (!in.joints.empty() && !in.weights.empty()) {
		for (auto const [joint, weight] : zip_ranges(in.joints[0], in.weights[0])) {
			ret.bones.push_back(graphics::Bone{
				.joint = to_glm_vec<4, std::uint32_t>(joint),
				.weight = to_glm_vec<4, float>(weight),
			});
		}
	}
	return ret;
}

struct NestedIndex {
	std::size_t index{};
	NestedIndex const* next{};
};

struct Exporter {
	// NOLINTNEXTLINE
	fs::path const& data_root;
	// NOLINTNEXTLINE
	fs::path const& prefix;
	// NOLINTNEXTLINE
	fs::path const& gltf_dir;
	// NOLINTNEXTLINE
	gltf2cpp::Root const& root;
	bool force{};

	[[nodiscard]] static auto make_filename(std::string_view name, std::string_view fallback, NestedIndex index, std::string_view suffix = {}) -> std::string {
		if (name.empty() || name == "(Unnamed)") { name = fallback; }
		auto ret = std::format("{}_{}", name, index.index);
		for (auto const* next = index.next; next != nullptr; next = next->next) { std::format_to(std::back_inserter(ret), "_{}", next->index); }
		ret += suffix;
		return ret;
	}

	[[nodiscard]] static auto write_file(std::span<std::uint8_t const> bytes, fs::path const& path) -> bool {
		fs::create_directories(path.parent_path());
		auto file = std::ofstream{path, std::ios::binary};
		if (!file) { return false; }
		// NOLINTNEXTLINE
		return static_cast<bool>(file.write(reinterpret_cast<char const*>(bytes.data()), static_cast<std::streamsize>(bytes.size_bytes())));
	}

	[[nodiscard]] static auto write_file(std::span<std::byte const> bytes, fs::path const& path) -> bool {
		// NOLINTNEXTLINE
		return write_file(std::span{reinterpret_cast<std::uint8_t const*>(bytes.data()), bytes.size_bytes()}, path);
	}

	[[nodiscard]] static auto export_failed(std::string_view asset_type, fs::path const& uri, std::size_t index) -> Error {
		return Error{std::format("failed to export {} [{}] at [{}]", asset_type, uri.generic_string(), index)};
	}

	[[nodiscard]] static auto exported(fs::path const& uri, char prefix = '-') -> std::string {
		return std::format("{} [{}] exported\n", prefix, uri.generic_string());
	}

	[[nodiscard]] static auto get_shader() -> graphics::Material::Shader {
		static auto const ret = graphics::LitMaterial{}.shader;
		return ret;
	}

	[[nodiscard]] static constexpr auto to_str(gltf2cpp::Wrap const wrap) {
		switch (wrap) {
		case gltf2cpp::Wrap::eClampEdge: return "clamp_edge";
		default: return "repeat";
		}
	}

	[[nodiscard]] static auto to_sampler(gltf2cpp::Sampler const& sampler) -> dj::Json {
		static constexpr auto from_wrap = [](gltf2cpp::Wrap const wrap) { return wrap == gltf2cpp::Wrap::eClampEdge ? "clamp_edge" : "repeat"; };
		static constexpr auto from_filter = [](gltf2cpp::Filter const filter) {
			switch (filter) {
			case gltf2cpp::Filter::eNearestMipmapLinear:
			case gltf2cpp::Filter::eNearestMipmapNearest:
			case gltf2cpp::Filter::eNearest: return "nearest";
			default: return "linear";
			}
		};

		auto ret = dj::Json{};
		ret["wrap_s"] = from_wrap(sampler.wrap_s);
		ret["wrap_t"] = from_wrap(sampler.wrap_t);
		if (sampler.min_filter) { ret["min"] = from_filter(*sampler.min_filter); }
		if (sampler.mag_filter) { ret["mag"] = from_filter(*sampler.mag_filter); }

		return ret;
	}

	[[nodiscard]] auto should_skip(fs::path const& uri, fs::path& out_dst) const -> bool {
		out_dst = data_root / uri;
		if (fs::exists(out_dst)) {
			if (force) {
				if (!fs::remove(out_dst)) { throw Error{std::format("failed to remove existing asset: '{}'", uri.generic_string())}; }
				std::cout << std::format("- removed: [{}]\n", uri.generic_string());
				return false;
			}
			std::cout << std::format("- skipped: [{}]\n", uri.generic_string());
			return true;
		}
		fs::create_directories(out_dst.parent_path());
		return false;
	}

	[[nodiscard]] auto write_image_bytes(std::span<std::byte const> bytes, fs::path const& uri) const -> bool {
		auto dst = fs::path{};
		if (should_skip(uri, dst)) { return true; }
		return write_file(bytes, dst);
	}

	[[nodiscard]] auto copy_image_file(fs::path const& src, fs::path const& uri) const -> bool {
		auto dst = fs::path{};
		if (should_skip(uri, dst)) { return true; }
		return fs::copy_file(src, dst);
	}

	[[nodiscard]] auto export_image(std::size_t index) const -> fs::path {
		auto const& asset = root.images[index];
		if (asset.source_filename.empty()) {
			auto image_uri = prefix / "textures" / make_filename(asset.name, "texture", {index});
			if (!write_image_bytes(asset.bytes.span(), image_uri)) { throw export_failed("image", image_uri, index); }
			return image_uri;
		}
		auto src_filename = fs::path{asset.source_filename}.filename();
		auto filename = make_filename(src_filename.stem().string(), {}, {index}, src_filename.extension().string());
		auto uri = prefix / "textures" / filename;
		if (!copy_image_file(gltf_dir / asset.source_filename, uri)) { throw export_failed("image", uri, index); }
		std::cout << exported(uri);
		return uri;
	}

	[[nodiscard]] auto export_texture(std::size_t index) const -> fs::path {
		auto const& asset = root.textures[index];
		auto const& image_asset = root.images[asset.source];
		auto uri = prefix / "textures" / make_filename(asset.name, fs::path{image_asset.source_filename}.stem().string(), {index}, ".json");
		auto dst = fs::path{};
		if (should_skip(uri, dst)) { return uri; }

		auto const image_uri = export_image(asset.source);
		auto json = dj::Json{};
		json["asset_type"] = TextureAsset::type_name_v;
		json["image"] = image_uri.generic_string();
		json["colour_space"] = asset.linear ? "linear" : "sRGB";

		if (asset.sampler) {
			auto const& sampler = root.samplers[*asset.sampler];
			json["sampler"] = to_sampler(sampler);
		}
		if (!json.to_file(dst.string().c_str())) { throw export_failed("texture", uri, index); }
		std::cout << exported(uri);
		return uri;
	}

	[[nodiscard]] auto export_material(std::size_t index) const -> fs::path {
		auto const& asset = root.materials[index];
		auto uri = prefix / "materials" / make_filename(asset.name, "material", {index}, ".json");
		auto dst = fs::path{};
		if (should_skip(uri, dst)) { return uri; }

		auto json = dj::Json{};
		json["asset_type"] = MaterialAsset::type_name_v;
		json["material_type"] = graphics::LitMaterial::material_type_v;
		json["shader"] = [&] {
			auto ret = dj::Json{};
			ret["vertex"] = get_shader().vertex.value();
			ret["fragment"] = get_shader().fragment.value();
			return ret;
		}();
		if (asset.pbr.base_color_texture) { json["base_colour"] = export_texture(asset.pbr.base_color_texture->texture).generic_string(); }
		if (asset.pbr.metallic_roughness_texture) {
			json["metallic_roughness"] = export_texture(asset.pbr.metallic_roughness_texture->texture).generic_string();
		}
		if (asset.emissive_texture) { json["emissive"] = export_texture(asset.emissive_texture->texture).generic_string(); }
		for (auto const i : asset.pbr.base_color_factor) { json["albedo"].push_back(i); }
		for (auto const i : asset.emissive_factor) { json["emissive_factor"].push_back(i); }
		json["metallic"] = asset.pbr.metallic_factor;
		json["roughness"] = asset.pbr.roughness_factor;
		json["alpha_cutoff"] = asset.alpha_cutoff;
		switch (asset.alpha_mode) {
		case gltf2cpp::AlphaMode::eBlend: json["alpha_mode"] = "blend"; break;
		case gltf2cpp::AlphaMode::eMask: json["blend_mode"] = "mask"; break;
		case gltf2cpp::AlphaMode::eOpaque: json["blend_mode"] = "opaque"; break;
		default: break;
		}
		if (!json.to_file(dst.string().c_str())) { throw export_failed("material", uri, index); }
		std::cout << exported(uri);
		return uri;
	}

	[[nodiscard]] auto export_geometry(gltf2cpp::Geometry const& in, std::size_t imesh, std::size_t igeometry) const -> fs::path {
		auto const index_geometry = NestedIndex{.index = igeometry};
		auto uri = prefix / "meshes" / make_filename("geometry", {}, {.index = imesh, .next = &index_geometry}, ".bin");
		auto dst = fs::path{};
		if (should_skip(uri, dst)) { return uri; }

		auto geometry = to_geometry(in);
		auto pack = bin::Pack{};
		pack.pack<graphics::Vertex>(geometry.vertices).pack<std::uint32_t>(geometry.indices).pack<graphics::Bone>(geometry.bones);
		if (!write_file(pack.bytes, dst.string())) { throw export_failed("geometry", uri, igeometry); }
		std::cout << exported(uri);
		return uri;
	}

	[[nodiscard]] auto export_static_mesh(std::size_t index) const -> fs::path {
		auto const& asset = root.meshes[index];
		auto uri = prefix / "meshes" / make_filename(asset.name, "mesh", {index}, ".json");
		auto dst = fs::path{};
		if (should_skip(uri, dst)) { return uri; }

		auto json = dj::Json{};
		json["asset_type"] = MeshAsset::type_name_v;
		json["mesh_type"] = "static";
		for (auto const [primitive, iprim] : enumerate(asset.primitives)) {
			auto& out_primitive = json["primitives"].push_back({});
			out_primitive["geometry"] = export_geometry(primitive.geometry, index, iprim).generic_string();
			if (primitive.material) { out_primitive["material"] = export_material(*primitive.material).generic_string(); }
		}
		if (!json.to_file(dst.string().c_str())) { throw export_failed("mesh", uri, index); }

		std::cout << exported(uri, '=');
		return uri;
	}
};

struct App {
	fs::path gltf_path{};
	fs::path data_root{fs::current_path()};
	std::vector<std::size_t> mesh_ids{};
	bool verbose{};
	bool force{};

	gltf2cpp::Root root{};
	fs::path gltf_dir{};
	fs::path export_prefix{};
	MeshList list{};

	template <typename T>
	static auto print_assets(std::vector<T> const& vec, std::string_view const type) {
		if (!vec.empty()) {
			std::cout << std::format("{}:\n", type);
			for (auto const [asset, index] : enumerate(vec)) {
				auto const name = asset.name.empty() ? std::string_view{"(unnamed)"} : asset.name;
				std::cout << std::format("[{}] {}\n", index, name);
			}
			std::cout << "\n";
		}
	}

	auto print_assets() const -> void {
		auto const print_meshes = [this](std::span<std::size_t const> indices, std::string_view const type) {
			if (!indices.empty()) {
				std::cout << std::format("{}:\n", type);
				for (auto const index : indices) {
					auto const& mesh = root.meshes[index];
					auto const name = mesh.name.empty() ? std::string_view{"(unnamed)"} : mesh.name;
					std::cout << std::format("[{}] {}\n", index, name);
				}
				std::cout << "\n";
			}
		};

		print_meshes(list.static_meshes, "static meshes");
		print_meshes(list.skinned_meshes, "skinned meshes");
		print_assets(root.skins, "skeletons");
		print_assets(root.materials, "materials");
		print_assets(root.textures, "textures");
		print_assets(root.animations, "skeletal animations");
	}

	[[nodiscard]] auto validate_mesh_ids() const -> bool {
		for (auto const index : mesh_ids) {
			if (!list.is_static(index) && !list.is_skinned(index)) {
				std::cerr << std::format("invalid mesh id - '{}'\n", index);
				print_assets();
				return false;
			}
		}
		return true;
	}

	[[nodiscard]] auto run() -> int {
		data_root = fs::absolute(data_root);
		gltf_dir = gltf_path.parent_path();
		export_prefix = gltf_path.stem();

		if (verbose) {
			std::cout << std::format(" - data root: '{}'\n - gltf path: '{}'\n - export prefix: '{}'\n - force: '{}'\n", data_root.generic_string(),
									 gltf_path.generic_string(), export_prefix.string(), force);
			std::cout << " - mesh-ids: ";
			bool first{true};
			for (auto const& id : mesh_ids) {
				if (!first) { std::cout << ", "; }
				first = false;
				std::cout << id;
			}
			std::cout << "\n";
		}

		root = gltf2cpp::parse(gltf_path.string().c_str());
		if (!root) {
			std::cerr << std::format("failed to parse '{}'\n", gltf_path.generic_string());
			return EXIT_FAILURE;
		}

		if (root.meshes.empty()) {
			std::cout << std::format("'{}' has no meshes\n", gltf_path.generic_string());
			return EXIT_SUCCESS;
		}

		list = MeshList::build(root);
		if (!validate_mesh_ids()) { return EXIT_FAILURE; }

		if (mesh_ids.empty()) {
			print_assets();
			return EXIT_SUCCESS;
		}

		fs::create_directories(data_root / export_prefix);
		auto const exporter = Exporter{data_root, export_prefix, gltf_dir, root, force};
		for (auto const index : mesh_ids) {
			std::cout << std::format("\nexporting mesh: [{}] {}...\n", index, root.meshes[index].name);
			auto const uri = exporter.export_static_mesh(index);
		}

		return EXIT_SUCCESS;
	}
};
} // namespace
} // namespace spaced::importer

auto main(int argc, char** argv) -> int {
	auto options = clap::Options{clap::make_app_name(*argv), "spaced mesh importer", SPACED_IMPORTER_VERSION};

	auto app = spaced::importer::App{};

	options.positional(app.gltf_path, "gltf-path", "gltf-path")
		.required(app.data_root, "d,data-root", "data root to export to", ".")
		.unmatched(app.mesh_ids, "[mesh-id...]")
		.flag(app.verbose, "v, verbose", "verbose mode")
		.flag(app.force, "f,force", "force export (remove existing assets)");

	auto const result = options.parse(argc, argv);
	if (clap::should_quit(result)) { return clap::return_code(result); }

	try {
		return app.run();
	} catch (spaced::Error const& e) {
		std::cerr << std::format("{}\n", e.what());
		return EXIT_FAILURE;
	} catch (std::exception const& e) {
		std::cerr << std::format("fatal error: {}\n", e.what());
		return EXIT_FAILURE;
	}
}
