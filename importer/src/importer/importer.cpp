#include <importer/importer.hpp>
#include <spaced/engine/core/enumerate.hpp>
#include <spaced/engine/core/ptr.hpp>
#include <spaced/engine/core/visitor.hpp>
#include <spaced/engine/core/zip_ranges.hpp>
#include <spaced/engine/error.hpp>
#include <spaced/engine/node_tree_serializer.hpp>
#include <spaced/engine/resources/animation_asset.hpp>
#include <spaced/engine/resources/bin_data.hpp>
#include <spaced/engine/resources/material_asset.hpp>
#include <spaced/engine/resources/mesh_asset.hpp>
#include <spaced/engine/resources/primitive_asset.hpp>
#include <spaced/engine/resources/skeleton_asset.hpp>
#include <spaced/engine/resources/texture_asset.hpp>
#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>

namespace spaced::importer {
namespace {
template <typename T>
auto print_asset_list(std::vector<T> const& list, std::string_view const type) {
	if (!list.empty()) {
		std::cout << std::format("{}:\n", type);
		for (auto const [asset, index] : enumerate(list)) {
			auto const name = asset.name.empty() ? std::string_view{"(unnamed)"} : asset.name;
			std::cout << std::format("[{}] {}\n", index, name);
		}
		std::cout << "\n";
	}
}
} // namespace

// TODO: split up
namespace {
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

[[nodiscard]] auto to_mat4(gltf2cpp::Mat4x4 const& in) -> glm::mat4 {
	auto ret = glm::mat4{};
	// NOLINTNEXTLINE
	std::memcpy(&ret[0], &in[0], sizeof(ret[0]));
	std::memcpy(&ret[1], &in[1], sizeof(ret[1]));
	std::memcpy(&ret[2], &in[2], sizeof(ret[2]));
	std::memcpy(&ret[3], &in[3], sizeof(ret[3]));
	return ret;
}

[[nodiscard]] auto to_transform(gltf2cpp::Transform const& transform) -> Transform {
	auto ret = Transform{};
	auto visitor = Visitor{
		[&ret](gltf2cpp::Trs const& trs) {
			ret.set_position({trs.translation[0], trs.translation[1], trs.translation[2]});
			ret.set_orientation({trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]});
			ret.set_scale({trs.scale[0], trs.scale[1], trs.scale[2]});
		},
		[&ret](gltf2cpp::Mat4x4 const& mat) { ret.decompose(to_mat4(mat)); },
	};
	std::visit(visitor, transform);
	return ret;
}

struct NestedIndex {
	std::size_t index{};
	NestedIndex const* next{};
};

struct Traverse {
	std::span<gltf2cpp::Node const> nodes{};

	template <typename FuncT>
	auto operator()(std::span<std::size_t const> roots, FuncT func) const {
		for (auto const index : roots) {
			if (!traverse(index, func)) { return; }
		}
	}

	template <typename FuncT>
	[[nodiscard]] auto traverse(std::size_t index, FuncT func) const -> bool {
		if (!keep_traversing(index, func)) { return false; }
		auto const& node = nodes[index];
		return std::ranges::all_of(node.children, [&](std::size_t index) { return traverse(index, func); });
	}

	template <typename FuncT>
	[[nodiscard]] auto keep_traversing(std::size_t index, FuncT func) const -> bool {
		auto const& node = nodes[index];
		if constexpr (std::same_as<std::invoke_result_t<FuncT, gltf2cpp::Node, std::size_t>, bool>) {
			return std::invoke(func, node, index);
		} else {
			std::invoke(func, node, index);
			return true;
		}
	}
};

[[nodiscard]] auto make_node_tree(std::span<gltf2cpp::Node const> nodes, std::span<std::size_t const> roots) -> NodeTree {
	auto map = NodeTree::DataMap{};
	auto const func = [&](gltf2cpp::Node const& in_node, [[maybe_unused]] std::size_t index) {
		auto parent = std::optional<Id<Node>>{};
		if (in_node.parent) { parent = *in_node.parent; }
		auto node = NodeTree::Data{};
		node.id = index;
		if (in_node.parent) { node.parent = *in_node.parent; }
		node.name = in_node.name;
		node.transform = to_transform(in_node.transform);
		node.children = {in_node.children.begin(), in_node.children.end()};
		map.insert_or_assign(index, std::move(node));
	};
	Traverse{nodes}(roots, func);
	auto ret = NodeTree{};
	ret.import_tree(std::move(map), {roots.begin(), roots.end()});
	return ret;
}

[[nodiscard]] constexpr auto to_interpolation(gltf2cpp::Interpolation in) -> graphics::Interpolation {
	switch (in) {
	case gltf2cpp::Interpolation::eCubicSpline: std::cout << "CubicSpline interpolation not supported\n"; return graphics::Interpolation::eLinear;
	default:
	case gltf2cpp::Interpolation::eLinear: return graphics::Interpolation::eLinear;
	case gltf2cpp::Interpolation::eStep: return graphics::Interpolation::eStep;
	}
}

template <typename T>
[[nodiscard]] auto to_interpolator(std::span<float const> times, std::span<T const> values, gltf2cpp::Interpolation interpolation)
	-> graphics::Interpolator<T> {
	assert(times.size() == values.size());
	auto ret = graphics::Interpolator<T>{};
	for (auto [t, v] : zip_ranges(times, values)) { ret.keyframes.push_back({v, Duration{t}}); }
	ret.interpolation = to_interpolation(interpolation);
	return ret;
}

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

	[[nodiscard]] auto export_material(std::size_t index, bool skinned) const -> fs::path {
		auto const& asset = root.materials[index];
		auto uri = prefix / "materials" / make_filename(asset.name, "material", {index}, ".json");
		auto dst = fs::path{};
		if (should_skip(uri, dst)) { return uri; }

		auto json = dj::Json{};
		json["asset_type"] = MaterialAsset::type_name_v;
		json["material_type"] = skinned ? graphics::SkinnedMaterial::material_type_v : graphics::LitMaterial::material_type_v;
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
		auto uri = prefix / "geometries" / make_filename("geometry", {}, {.index = imesh, .next = &index_geometry}, ".bin");
		auto dst = fs::path{};
		if (should_skip(uri, dst)) { return uri; }

		auto const geometry = to_geometry(in);
		auto bytes = std::vector<std::uint8_t>{};
		PrimitiveAsset::bin_pack_to(bytes, geometry);
		if (!write_file(bytes, dst.string())) { throw export_failed("geometry", uri, igeometry); }
		std::cout << exported(uri);
		return uri;
	}

	[[nodiscard]] auto make_animation_channel(gltf2cpp::Animation::Channel const& in, Id<Node> node, gltf2cpp::Animation const& animation) const
		-> std::optional<graphics::Animation::Channel> {
		auto const& sampler = animation.samplers[in.sampler];
		auto ret = graphics::Animation::Channel{node};
		auto const& input = root.accessors[sampler.input];
		assert(input.type == gltf2cpp::Accessor::Type::eScalar && input.component_type == gltf2cpp::ComponentType::eFloat);
		auto times = std::get<gltf2cpp::Accessor::Float>(input.data).span();
		auto const& output = root.accessors[sampler.output];
		assert(output.component_type == gltf2cpp::ComponentType::eFloat);
		auto const values = std::get<gltf2cpp::Accessor::Float>(output.data).span();
		auto const translation_or_scale = [&]() {
			assert(output.type == gltf2cpp::Accessor::Type::eVec3);
			auto vec = std::vector<glm::vec3>(values.size() / 3);
			std::memcpy(vec.data(), values.data(), values.size_bytes());
			return vec;
		};
		switch (in.target.path) {
		case gltf2cpp::Animation::Path::eTranslation: {
			auto vec = translation_or_scale();
			ret.storage = graphics::Animation::Translate{to_interpolator<glm::vec3>(times, vec, sampler.interpolation)};
			break;
		}
		case gltf2cpp::Animation::Path::eScale: {
			auto const vec = translation_or_scale();
			ret.storage = graphics::Animation::Scale{to_interpolator<glm::vec3>(times, vec, sampler.interpolation)};
			break;
		}
		case gltf2cpp::Animation::Path::eRotation: {
			assert(output.type == gltf2cpp::Accessor::Type::eVec4);
			auto vec = std::vector<glm::quat>(values.size() / 4);
			std::memcpy(vec.data(), values.data(), values.size_bytes());
			ret.storage = graphics::Animation::Rotate{to_interpolator<glm::quat>(times, vec, sampler.interpolation)};
			break;
		}
		default: std::cout << " - unsupported animation path\n"; return {};
		}
		return ret;
	}

	[[nodiscard]] auto export_animation(NodeLocator node_locator, std::size_t index) const -> fs::path {
		auto const& asset = root.animations[index];
		auto out_animation = graphics::Animation{};
		for (auto const [in_channel, channel_index] : enumerate(asset.channels)) {
			if (!in_channel.target.node) { continue; }
			auto const* node = node_locator.find(*in_channel.target.node);
			if (node == nullptr) {
				throw Error{std::format("dangling joint [{}] on animation [{}] channel [{}]", *in_channel.target.node, index, channel_index)};
			}
			auto out_channel = make_animation_channel(in_channel, node->id(), asset);
			if (!out_channel) { continue; }
			out_animation.channels.push_back(std::move(*out_channel));
		}
		if (out_animation.channels.empty()) {
			std::cout << std::format("- no channels in animation [{}]\n", index);
			return {};
		}

		auto uri = prefix / "animations" / make_filename(asset.name, "animation", {index}, ".bin");
		auto dst = fs::path{};
		if (should_skip(uri, dst)) { return uri; }
		auto bytes = std::vector<std::uint8_t>{};
		AnimationAsset::bin_pack_to(bytes, out_animation);
		if (!write_file(bytes, dst)) { throw Error{std::format("failed to export animation [{}]", index)}; }

		std::cout << exported(uri);
		return uri;
	}

	[[nodiscard]] auto is_node_in_hierarchy(gltf2cpp::Node const& node, std::size_t index) const -> bool {
		return std::ranges::any_of(node.children, [&](std::size_t child_index) {
			if (child_index == index) { return true; }
			return is_node_in_hierarchy(root.nodes[child_index], index);
		});
	}

	[[nodiscard]] auto find_scene_with_node(std::size_t const index) const -> Ptr<gltf2cpp::Scene const> {
		for (auto const& scene : root.scenes) {
			for (auto const root_node : scene.root_nodes) {
				if (is_node_in_hierarchy(root.nodes[root_node], index)) { return &scene; }
			}
		}
		return {};
	}

	[[nodiscard]] auto get_root_node_indices(std::size_t skin_index) const -> std::span<std::size_t const> {
		auto const& skin = root.skins[skin_index];
		if (root.scenes.empty()) {
			throw Error{std::format("cannot export skeleton [{}]: GLTF skin missing skeleton property, and GLTF data has no scenes", skin_index)};
		}

		auto ret = std::span<std::size_t const>{};
		if (skin.skeleton) {
			auto const* scene = find_scene_with_node(*skin.skeleton);
			if (scene == nullptr) { throw Error{std::format("failed to locate scene with skeleton node [{}]", *skin.skeleton)}; }
			ret = scene->root_nodes;
		} else {
			ret = root.scenes.front().root_nodes;
		}
		return ret;
	}

	[[nodiscard]] auto export_skeleton(std::size_t skin_index) const -> fs::path {
		auto const& skin = root.skins[skin_index];
		auto uri = prefix / "skeletons" / make_filename(skin.name, "skeleton", {skin_index}, ".json");
		auto dst = fs::path{};
		if (should_skip(uri, dst)) { return uri; }

		auto const root_node_indices = get_root_node_indices(skin_index);
		auto skeleton = graphics::Skeleton{.joint_tree = make_node_tree(root.nodes, root_node_indices)};

		for (auto const in_joint_id : skin.joints) {
			auto const* node = skeleton.joint_tree.find(in_joint_id);
			if (node == nullptr) {
				// TODO more details in message
				throw Error{std::format("dangling joint index: [{}]", in_joint_id)};
			}
			assert(in_joint_id == node->id());
			skeleton.ordered_joint_ids.push_back(node->id());
		}
		if (skin.inverse_bind_matrices) {
			auto const ibm = root.accessors[*skin.inverse_bind_matrices].to_mat4();
			assert(ibm.size() >= skin.joints.size());
			skeleton.inverse_bind_matrices.reserve(ibm.size());
			for (auto const& mat : ibm) {
				skeleton.inverse_bind_matrices.push_back(to_mat4(mat));
				if (skeleton.inverse_bind_matrices.size() == skin.joints.size()) { break; }
			}
		} else {
			skeleton.inverse_bind_matrices = std::vector<glm::mat4>(skin.joints.size(), glm::identity<glm::mat4x4>());
		}
		auto json = dj::Json{};
		json["asset_type"] = SkeletonAsset::type_name_v;
		NodeTree::Serializer::serialize(json["joint_tree"], skeleton.joint_tree);
		for (auto const joint_id : skeleton.ordered_joint_ids) { json["ordered_joint_ids"].push_back(joint_id.value()); }
		for (auto const& ibm : skeleton.inverse_bind_matrices) { io::to_json(json["inverse_bind_matrices"].push_back({}), ibm); }
		auto node_locator = NodeLocator{skeleton.joint_tree};
		for (std::size_t index = 0; index < root.animations.size(); ++index) {
			json["animations"].push_back(export_animation(node_locator, index).generic_string());
		}

		if (!json.to_file(dst.string().c_str())) { throw Error{std::format("failed to save skeleton (skin) [{}]", skin_index)}; }

		std::cout << exported(uri);
		return uri;
	}

	[[nodiscard]] auto make_mesh(bool skinned, std::size_t index) const -> dj::Json {
		auto const& asset = root.meshes[index];
		auto ret = dj::Json{};
		ret["asset_type"] = MeshAsset::type_name_v;
		ret["mesh_type"] = skinned ? "skinned" : "static";
		for (auto const [primitive, iprim] : enumerate(asset.primitives)) {
			auto& out_primitive = ret["primitives"].push_back({});
			out_primitive["geometry"] = export_geometry(primitive.geometry, index, iprim).generic_string();
			if (primitive.material) { out_primitive["material"] = export_material(*primitive.material, skinned).generic_string(); }
		}

		return ret;
	}

	[[nodiscard]] auto export_mesh(std::size_t index, Ptr<gltf2cpp::Node const> node) const -> fs::path {
		auto const& asset = root.meshes[index];
		auto uri = prefix / make_filename(asset.name, "mesh", {index}, ".json");
		auto dst = fs::path{};
		if (should_skip(uri, dst)) { return uri; }

		auto json = make_mesh(node != nullptr && node->skin.has_value(), index);
		if (node != nullptr && node->skin) { json["skeleton"] = export_skeleton(*node->skin).generic_string(); }

		if (!json.to_file(dst.string().c_str())) { throw export_failed("mesh", uri, index); }

		std::cout << exported(uri, '=');
		return uri;
	}
};
} // namespace

auto Importer::MeshList::build(gltf2cpp::Root const& root) -> MeshList {
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

auto Importer::run(Input input) -> int {
	m_input = std::move(input);
	m_input.data_root = fs::absolute(m_input.data_root);
	m_gltf_dir = m_input.gltf_path.parent_path();
	m_export_prefix = m_input.uri_prefix / m_input.gltf_path.stem();

	auto mesh_id = std::size_t{};
	if (!m_input.meshes.empty()) {
		if (m_input.meshes.size() > 1) {
			std::cerr << " importing more than one mesh is not supported\n";
			return EXIT_FAILURE;
		}
		mesh_id = m_input.meshes.front();
	}

	if (m_input.verbose) {
		std::cout << std::format(" - data root: '{}'\n - gltf path: '{}'\n - export prefix: '{}'\n - force: '{}'\n - mesh: [{}]\n",
								 m_input.data_root.generic_string(), m_input.gltf_path.generic_string(), m_export_prefix.string(), m_input.force, mesh_id);
	}

	m_root = gltf2cpp::parse(m_input.gltf_path.string().c_str());
	if (!m_root) {
		std::cerr << std::format("failed to parse '{}'\n", m_input.gltf_path.generic_string());
		return EXIT_FAILURE;
	}

	if (m_root.meshes.empty()) {
		std::cout << std::format("'{}' has no meshes\n", m_input.gltf_path.generic_string());
		return EXIT_SUCCESS;
	}

	m_mesh_list = MeshList::build(m_root);
	if (m_input.list) {
		print_assets();
		return EXIT_SUCCESS;
	}

	if (mesh_id >= m_root.meshes.size()) {
		std::cerr << std::format("invalid mesh - '{}'\n", mesh_id);
		return EXIT_FAILURE;
	}

	fs::create_directories(m_input.data_root / m_export_prefix);
	auto const exporter = Exporter{m_input.data_root, m_export_prefix, m_gltf_dir, m_root, m_input.force};
	auto const* node = Ptr<gltf2cpp::Node const>{};
	if (std::ranges::find(m_mesh_list.skinned_meshes, mesh_id) != m_mesh_list.skinned_meshes.end()) {
		for (auto const& in_node : m_root.nodes) {
			if (!in_node.mesh || *in_node.mesh != mesh_id) { continue; }
			assert(in_node.skin);
			node = &in_node;
			break;
		}
		if (node == nullptr) { throw Error{std::format("failed to find skin for skinned mesh [{}]", mesh_id)}; }
	}
	std::cout << std::format("\nexporting mesh: [{}] {}...\n", mesh_id, m_root.meshes[mesh_id].name);
	auto const uri = exporter.export_mesh(mesh_id, node);

	return EXIT_SUCCESS;
}

auto Importer::print_assets() const -> void {
	auto const print_meshes = [this](std::span<std::size_t const> indices, std::string_view const type) {
		if (!indices.empty()) {
			std::cout << std::format("{}:\n", type);
			for (auto const index : indices) {
				auto const& mesh = m_root.meshes[index];
				auto const name = mesh.name.empty() ? std::string_view{"(unnamed)"} : mesh.name;
				std::cout << std::format("[{}] {}\n", index, name);
			}
			std::cout << "\n";
		}
	};

	print_meshes(m_mesh_list.static_meshes, "static meshes");
	print_meshes(m_mesh_list.skinned_meshes, "skinned meshes");
	print_asset_list(m_root.skins, "skeletons");
	print_asset_list(m_root.materials, "materials");
	print_asset_list(m_root.textures, "textures");
	print_asset_list(m_root.animations, "skeletal animations");
}
} // namespace spaced::importer
