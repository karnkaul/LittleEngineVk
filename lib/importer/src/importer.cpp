#include <glm/vec4.hpp>
#include <gltf2cpp/gltf2cpp.hpp>
#include <levk/assets/bin_animation_channels.hpp>
#include <levk/assets/bin_geometry.hpp>
#include <levk/assets/bin_transform_sampler.hpp>
#include <levk/assets/lit_material_asset.hpp>
#include <levk/assets/mesh_asset.hpp>
#include <levk/assets/primitive_asset.hpp>
#include <levk/assets/scene_info_asset.hpp>
#include <levk/assets/skeletal_animation_asset.hpp>
#include <levk/assets/skeleton_asset.hpp>
#include <levk/assets/texture_asset.hpp>
#include <levk/core/error.hpp>
#include <levk/core/string_hash.hpp>
#include <levk/core/visitor.hpp>
#include <levk/importer.hpp>
#include <levk/io/binary_io.hpp>
#include <levk/io/json_io.hpp>
#include <levk/io/tree_io.hpp>
#include <levk/vfs.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace levk {
namespace fs = std::filesystem;

namespace {
template <typename Type, std::size_t Dim>
constexpr auto to_glm_vec(gltf2cpp::TVec<Type, Dim> const& in) {
	auto ret = glm::vec<glm::length_t(Dim), Type>{};
	ret.x = in[0];
	if constexpr (Dim > 1) { ret.y = in[1]; }
	if constexpr (Dim > 2) { ret.z = in[2]; }
	if constexpr (Dim > 3) { ret.w = in[3]; }
	return ret;
}

auto to_glm_quat(gltf2cpp::Vec<4> const& in) {
	auto ret = glm::quat{};
	std::memcpy(&ret, in.data(), sizeof(glm::quat));
	return ret;
}

auto to_glm_mat4(gltf2cpp::Mat4x4 const& in) {
	return glm::mat4{
		to_glm_vec(in[0]),
		to_glm_vec(in[1]),
		to_glm_vec(in[2]),
		to_glm_vec(in[3]),
	};
}

[[nodiscard]] constexpr auto to_rgba(gltf2cpp::Vec<4> const& in) { return colour::to_u8(to_glm_vec(in)); }
[[nodiscard]] constexpr auto to_rgba(gltf2cpp::Vec<3> const& in) { return RgbaU8{colour::to_u8(to_glm_vec(in)), colour::max_channel_u8}; }

[[nodiscard]] auto do_create_directories(fs::path const& path) {
	auto error_code = std::error_code{};
	if (fs::exists(path, error_code)) { return true; }
	return fs::create_directories(path, error_code);
}

[[nodiscard]] auto do_copy_file(fs::path const& src, fs::path const& dst) {
	auto error_code = std::error_code{};
	if (!do_create_directories(dst.parent_path())) { return false; }
	if (fs::exists(dst, error_code) && !fs::remove(dst, error_code)) { return false; }
	return fs::copy_file(src, dst, error_code);
}

[[nodiscard]] auto write_bytes(fs::path const& path, std::span<std::byte const> bytes) {
	if (!do_create_directories(path.parent_path())) { return false; }
	auto file = std::ofstream{path, std::ios::binary};
	if (!file) { return false; }
	for (auto const byte : bytes) { file << static_cast<char>(byte); }
	return true;
}

[[nodiscard]] auto write_json(dj::Json const& json, fs::path const& dst) {
	if (!do_create_directories(dst.parent_path())) { return false; }
	return json.to_file(dst.string().c_str());
}
} // namespace

class Importer::Impl {
  public:
	auto get_import_asset() -> ImportAsset const& {
		ensure_root();
		return m_import_asset;
	}

	auto get_mesh_name(ImportIndex const index) -> std::string_view {
		if (!ensure_root() || !check_index(m_root->meshes, to_size_t(index))) { return {}; }
		return m_root->meshes.at(to_size_t(index)).name;
	}

	auto get_skeleton_name(ImportIndex const index) -> std::string_view {
		if (!ensure_root() || !check_index(m_root->meshes, to_size_t(index))) { return {}; }
		return m_root->skins.at(to_size_t(index)).name;
	}

	auto import_node(ImportIndex const index) -> dj::Json {
		if (!ensure_root() || !check_index(m_root->nodes, to_size_t(index))) { return {}; }

		auto const& node = m_root->nodes.at(to_size_t(index));
		auto ret = dj::Json{};
		ret["import_index"] = node.self;
		ret["name"] = get_node_name(node);

		auto const transform = std::visit(GetTransform{}, m_root->nodes.at(to_size_t(index)).transform);
		to_json(ret["transform"], transform.get_data());
		if (node.parent) { ret["parent"] = *node.parent; }
		for (auto const index : node.children) { ret["children"].push_back(static_cast<std::int64_t>(index)); }

		if (node.mesh) {
			auto const uri = import_mesh(*node.mesh);
			if (!uri.empty()) { ret["mesh"] = uri; }
		}
		if (node.skin) {
			auto const uri = import_skeleton(*node.skin);
			if (!uri.empty()) { ret["skeleton"] = uri; }
		}
		if (node.camera) {
			auto camera = export_camera(*node.camera);
			if (camera) { ret["camera"] = std::move(camera); }
		}

		return ret;
	}

	auto import_scene(ImportIndex const index) -> std::string {
		if (!ensure_root() || !check_index(m_root->scenes, to_size_t(index))) { return {}; }

		auto const& scene = m_root->scenes.at(to_size_t(index));
		auto const name = std::format("scene_{}.json", to_size_t(index));
		auto dst_uri = get_uri(name, "scenes");
		if (!should_import(dst_uri)) { return dst_uri; }

		auto json = dj::Json{};
		json["type_name"] = get_type_name<SceneInfoAsset>();
		for (auto const index : scene.root_nodes) {
			add_node_and_children(to_import_index(index), json["nodes"]);
			json["root_nodes"].push_back(index);
		}

		if (!write_json(json, get_path(dst_uri))) {
			m_log.error("failed to save Scene: '{}'", dst_uri);
			return {};
		}

		return success(std::move(dst_uri), "Scene");
	}

  private:
	using AnimChannel = gltf2cpp::Animation::Channel;

	template <typename T>
	struct ImportInfo {
		Ptr<T const> asset{};
		std::string name{};
		std::string dst_uri{};

		explicit operator bool() const { return asset != nullptr; }
	};

	struct GetTransform {
		[[nodiscard]] auto operator()(gltf2cpp::Trs const& trs) const -> Transform {
			auto ret = Transform{};
			ret.set_position(to_glm_vec(trs.translation));
			ret.set_orientation(to_glm_quat(trs.rotation));
			ret.set_scale(to_glm_vec(trs.scale));
			return ret;
		}

		[[nodiscard]] auto operator()(gltf2cpp::Mat4x4 const& mat4) const -> Transform {
			auto const mat = to_glm_mat4(mat4);
			auto ret = Transform{};
			ret.from_matrix(mat);
			return ret;
		}
	};

	struct AddNodeAndParents {
		std::span<gltf2cpp::Node const> nodes;

		std::unordered_map<ImportIndex, TreeNodeId> added{};

		// NOLINTNEXTLINE(misc-no-recursion)
		auto add_node_and_parents(gltf2cpp::Node const& in_node, NodeTree& out) -> TreeNodeId {
			if (auto const it = added.find(to_import_index(in_node.self)); it != added.end()) { return it->second; }
			auto node = TreeNode{};
			node.name = get_node_name(in_node);
			node.transform = std::visit(GetTransform{}, in_node.transform);
			auto const id = out.add_node(std::move(node), to_import_index(in_node.self)).get_id();
			if (in_node.parent) {
				auto const parent_id = add_node_and_parents(nodes[*in_node.parent], out);
				out.set_parent(*out.get_node(id), parent_id);
			}
			added.insert_or_assign(to_import_index(in_node.self), id);
			return id;
		}

		void operator()(gltf2cpp::Node const& in_node, NodeTree& out) { add_node_and_parents(in_node, out); }
	};

	struct ShaderMapping {
		bool has_tangents{};
		bool has_normal_map{};
		bool has_joints{};

		[[nodiscard]] constexpr auto is_normal_mapping_enabled() const { return has_tangents && has_normal_map && !has_joints; }

		[[nodiscard]] constexpr auto get_vertex_shader(Shader const& shader) const {
			if (has_joints) { return shader.skinned_vertex_uri; }
			if (has_tangents && has_normal_map) { return shader.tbn_vertex_uri; }
			return shader.lit_vertex_uri;
		}

		[[nodiscard]] constexpr auto get_fragment_shader(Shader const& shader) const {
			if (!has_joints && has_tangents && has_normal_map) { return shader.tbn_fragment_uri; }
			return shader.lit_fragment_uri;
		}
	};

	[[nodiscard]] auto get_shader_mapping(gltf2cpp::Mesh::Primitive const& primitive) const -> ShaderMapping {
		auto ret = ShaderMapping{.has_tangents = !primitive.geometry.tangents.empty()};
		if (!primitive.material) {
			ret.has_normal_map = false;
		} else {
			ret.has_normal_map = m_root->materials.at(*primitive.material).normal_texture.has_value();
		}
		ret.has_joints = !primitive.geometry.joints.empty();
		if (m_shader.tbn_vertex_uri.empty() || m_shader.tbn_fragment_uri.empty()) {
			// disable normal mapping, use lit shaders.
			ret.has_normal_map = false;
		}
		return ret;
	}

	template <typename ContainerT>
	[[nodiscard]] auto get_info(ContainerT const& src, std::size_t index, std::string_view name, std::string_view subdir,
								std::string_view extension) -> ImportInfo<typename ContainerT::value_type> {
		if (!ensure_root() || !check_index(src, index)) { return {}; }

		auto ret = ImportInfo<typename ContainerT::value_type>{};
		ret.asset = &src[index];
		ret.name = get_name(ret.asset->name, std::format("{}_{}", name, index));
		ret.dst_uri = get_uri(ret.name, subdir);
		ret.dst_uri += extension;
		if (!should_import(ret.dst_uri)) { ret.asset = {}; }
		return ret;
	}

	auto try_export_json(dj::Json const& json, std::string dst_uri, std::string_view const type) -> std::string {
		if (!write_json(json, get_path(dst_uri))) {
			m_log.error("failed to save {}: '{}'", type, dst_uri);
			return {};
		}
		return success(std::move(dst_uri), type);
	}

	auto try_export_bin(std::span<std::byte const> bytes, std::string dst_uri, std::string_view const type) -> std::string {
		if (bytes.empty() || !write_bytes(get_path(dst_uri), bytes)) {
			m_log.error("failed to save {}: '{}'", type, dst_uri);
			return {};
		}
		return success(std::move(dst_uri), type);
	}

	auto import_image(std::size_t const index) -> std::string {
		if (!ensure_root() || !check_index(m_root->images, index)) { return {}; }

		auto const& image = m_root->images.at(index);
		if (!image.source_filename.empty()) {
			auto const src_path = fs::path{m_vfs.get_mount_point()} / image.source_filename;
			auto dst_uri = get_uri(image.source_filename, "images");
			if (!should_import(dst_uri)) { return dst_uri; }

			if (!do_copy_file(src_path, get_path(dst_uri))) {
				m_log.error("failed to copy image: '{}'", dst_uri);
				return {};
			}
			m_log.info("imported image: '{}'", dst_uri);
			return dst_uri;
		}

		auto dst_uri = get_uri(std::format("image_{}", index), "images");
		if (!should_import(dst_uri)) { return dst_uri; }

		return try_export_bin(image.bytes.span(), std::move(dst_uri), "Image");
	}

	auto import_texture(std::size_t const index, bool const mip_map, bool const linear) -> std::string {
		auto info = get_info(m_root->textures, index, "texture", "textures", ".json");
		if (!info) { return std::move(info.dst_uri); }
		auto const& texture = *info.asset;

		auto const image_uri = import_image(texture.source);
		if (image_uri.empty()) { return {}; }

		auto json = dj::Json{};
		json["type_name"] = get_type_name<TextureAsset>();
		json["name"] = info.name;
		json["image"] = image_uri;
		json["mip_map"] = mip_map ? dj::true_v : dj::false_v;
		json["linear"] = linear ? dj::true_v : dj::false_v;

		auto const to_filter = [](gltf2cpp::Filter const in) {
			switch (in) {
			case gltf2cpp::Filter::eNearest:
			case gltf2cpp::Filter::eNearestMipmapLinear:
			case gltf2cpp::Filter::eNearestMipmapNearest: return "nearest";
			default: return "linear";
			}
		};

		auto const to_wrap = [](gltf2cpp::Wrap const in) {
			switch (in) {
			case gltf2cpp::Wrap::eClampEdge: return "clamp_edge";
			case gltf2cpp::Wrap::eMirrorRepeat: return "mirror";
			default:
			case gltf2cpp::Wrap::eRepeat: return "repeat";
			}
		};

		if (texture.sampler) {
			auto const& in_sampler = m_root->samplers.at(*texture.sampler);
			auto& out_sampler = json["sampler"];
			if (in_sampler.mag_filter) { out_sampler["min_filter"] = to_filter(*in_sampler.mag_filter); }
			if (in_sampler.min_filter) { out_sampler["min_filter"] = to_filter(*in_sampler.min_filter); }
			out_sampler["wrap_u"] = to_wrap(in_sampler.wrap_s);
			out_sampler["wrap_v"] = to_wrap(in_sampler.wrap_t);
		}

		return try_export_json(json, std::move(info.dst_uri), "Texture");
	}

	auto import_material(ShaderMapping const& shader_mapping, std::size_t const index) -> std::string {
		auto info = get_info(m_root->materials, index, "material", "materials", ".json");
		return import_material(info, shader_mapping);
	}

	auto import_mesh(std::size_t const index) -> std::string {
		auto info = get_info(m_root->meshes, index, "mesh", "meshes", ".json");
		if (!info) { return std::move(info.dst_uri); }
		auto const& mesh = *info.asset;

		if (mesh.primitives.empty()) {
			m_log.error("Mesh '{}' has no primitives", info.dst_uri);
			return {};
		}

		auto json = dj::Json{};
		if (mesh.primitives.front().geometry.joints.empty()) {
			json["type_name"] = get_type_name<StaticMeshAsset>();
			json["name"] = info.name;
		} else {
			// this is a skinned mesh
			json["type_name"] = get_type_name<SkinnedMeshAsset>();
			json["name"] = info.name;
			auto const skin_index = find_skin_for(index);
			if (!skin_index) {
				m_log.error("could not find any Skeletons for SkinnedMesh: '{}'", info.dst_uri);
				return {};
			}
			auto const skeleton_uri = import_skeleton(*skin_index);
			if (skeleton_uri.empty()) {
				m_log.error("could not find any Skeletons for SkinnedMesh: '{}'", info.dst_uri);
				return {};
			}
			json["skeleton"] = skeleton_uri;
		}

		auto& primitives = json["primitives"];
		for (std::size_t index = 0; index < mesh.primitives.size(); ++index) {
			auto const primitive_uri = import_primitive(mesh.primitives.at(index), info.name, index);
			if (primitive_uri.empty()) { return {}; }
			primitives.push_back(primitive_uri);
		}

		return try_export_json(json, std::move(info.dst_uri), "Mesh");
	}

	auto import_skeletal_animation(std::size_t const index) -> std::string {
		auto info = get_info(m_root->animations, index, "animation", "skeletons", ".json");
		if (!info) { return std::move(info.dst_uri); }

		auto const& animation = *info.asset;
		auto channels_uri = import_animation_channels(animation, info.name);
		if (channels_uri.empty()) { return {}; }

		auto json = dj::Json{};
		json["type_name"] = get_type_name<SkeletalAnimationAsset>();
		json["name"] = info.name;
		json["channels"] = channels_uri;

		return try_export_json(json, std::move(info.dst_uri), "SkeletalAnimation");
	}

	auto import_skeleton(std::size_t const index) -> std::string {
		if (!ensure_root() || !check_index(m_root->skins, index)) { return {}; }

		auto info = get_info(m_root->skins, index, "skeleton", "skeletons", ".json");
		if (!info) { return std::move(info.dst_uri); }

		auto const& skin = m_root->skins.at(index);

		auto inverse_bind_matrices = dj::Json{};
		if (skin.inverse_bind_matrices) {
			auto const& accessor = m_root->accessors.at(*skin.inverse_bind_matrices);
			if (accessor.count < skin.joints.size()) {
				m_log.error("invalid inverse bind matrices for Skeleton: '{}'", info.dst_uri);
				return {};
			}
			auto const mats = accessor.to_mat4();
			for (auto const& in_mat : mats) {
				auto const mat = to_glm_mat4(in_mat);
				to_json(inverse_bind_matrices.push_back({}), mat);
			}
		}

		auto json = dj::Json{};
		json["type_name"] = get_type_name<SkeletonAsset>();
		json["name"] = info.name;
		if (skin.skeleton) {
			json["root_joint"]["import_index"] = *skin.skeleton;
			json["root_joint"]["name"] = get_node_name(m_root->nodes.at(*skin.skeleton));
		}
		json["name"] = info.name;

		for (std::size_t i = 0; i < m_root->animations.size(); ++i) {
			auto const animation_uri = import_skeletal_animation(i);
			if (animation_uri.empty()) { continue; }
			json["animations"].push_back(animation_uri);
		}

		auto tree = NodeTree{};
		auto add_node_and_parents = AddNodeAndParents{.nodes = m_root->nodes};

		for (auto const joint_index : skin.joints) {
			add_node_and_parents(m_root->nodes.at(joint_index), tree);
			json["joints_import_indices"].push_back(joint_index);
		}
		if (inverse_bind_matrices) { json["inverse_bind_matrices"] = std::move(inverse_bind_matrices); }
		json["joint_tree"] = TreeToJson{}.export_tree(tree);

		return try_export_json(json, std::move(info.dst_uri), "Skeleton");
	}

	auto import_vertex_array(gltf2cpp::Geometry const& geometry, std::string_view const mesh_name, std::size_t const index) -> std::string {
		auto const name = std::format("{}.vertex_array_{}.bin", mesh_name, index);
		auto dst_uri = get_uri(name, "primitives");
		if (!should_import(dst_uri)) { return dst_uri; }

		auto vertex_array = VertexArray{};
		vertex_array.vertices.reserve(geometry.positions.size());
		auto const* colours = geometry.colors.empty() ? nullptr : &geometry.colors.front();
		auto const* uvs = geometry.tex_coords.empty() ? nullptr : &geometry.tex_coords.front();
		for (std::size_t index = 0; index < geometry.positions.size(); ++index) {
			auto vertex = Vertex{.position = to_glm_vec(geometry.positions.at(index))};
			if (colours != nullptr && index < colours->size()) { vertex.rgba = {to_glm_vec(colours->at(index)), 1.0f}; }
			if (uvs != nullptr && index < uvs->size()) { vertex.uv = to_glm_vec(uvs->at(index)); }
			if (index < geometry.normals.size()) { vertex.normal = to_glm_vec(geometry.normals.at(index)); }
			if (index < geometry.tangents.size()) { vertex.tangent = to_glm_vec(geometry.tangents.at(index)); }
			vertex_array.vertices.push_back(vertex);
		}

		vertex_array.indices = geometry.indices;
		if (vertex_array.is_empty()) {
			m_log.error("no vertices in Primitive index '{}' on Mesh '{}'", index, mesh_name);
			return {};
		}

		auto bytes = std::vector<std::byte>{};
		to_bytes(bytes, vertex_array);

		return try_export_bin(bytes, std::move(dst_uri), "VertexArray");
	}

	auto import_vertex_skin(gltf2cpp::Geometry const& geometry, std::string_view const mesh_name, std::size_t const index) -> std::string {
		assert(!geometry.joints.empty() && !geometry.weights.empty());

		auto const& joints = geometry.joints.front();
		auto const& weights = geometry.weights.front();
		if (joints.empty() || weights.empty()) {
			m_log.error("no joints/weights in Primitive index '{}' on Mesh '{}'", index, mesh_name);
			return {};
		}

		auto const name = std::format("{}.vertex_skin_{}.bin", mesh_name, index);
		auto dst_uri = get_uri(name, "primitives");
		if (!should_import(dst_uri)) { return dst_uri; }

		auto skin = VertexSkin{};
		skin.joint_indices.reserve(geometry.joints.size());
		for (auto const& joint : joints) { skin.joint_indices.push_back(to_glm_vec(joint)); }
		skin.weights.reserve(weights.size());
		for (auto const& weight : weights) { skin.weights.push_back(to_glm_vec(weight)); }

		auto bytes = std::vector<std::byte>{};
		to_bytes(bytes, skin);

		return try_export_bin(bytes, std::move(dst_uri), "VertexSkin");
	}

	auto import_primitive(gltf2cpp::Mesh::Primitive const& primitive, std::string_view const mesh_name, std::size_t const index) -> std::string {
		auto dst_uri = get_uri(std::format("{}.primitive_{}.json", mesh_name, index), "primitives");
		if (!should_import(dst_uri)) { return dst_uri; }

		auto shader_mapping = get_shader_mapping(primitive);

		auto const material_uri = [&] {
			if (primitive.material) { return import_material(shader_mapping, *primitive.material); }
			// disable normal mapping.
			shader_mapping.has_normal_map = false;
			static auto const mat = gltf2cpp::Material{};
			auto const name = std::format("{}.material_{}", mesh_name, index);
			auto const info = get_info(std::span{&mat, 1}, 0, name, "materials", ".json");
			return import_material(info, shader_mapping);
		}();
		if (material_uri.empty()) { return {}; }

		auto const vertex_shader = shader_mapping.get_vertex_shader(m_shader);
		if (vertex_shader.empty()) {
			m_log.error("cannot import Primitive: '{}', vertex shader not provided", dst_uri);
			return {};
		}

		auto const vertex_array_uri = import_vertex_array(primitive.geometry, mesh_name, index);
		if (vertex_array_uri.empty()) { return {}; }

		auto const vertex_skin_uri = [&] {
			if (primitive.geometry.joints.empty()) { return std::string{}; }
			return import_vertex_skin(primitive.geometry, mesh_name, index);
		}();

		auto json = dj::Json{};
		if (!vertex_skin_uri.empty()) {
			json["type_name"] = get_type_name<SkinnedPrimitiveAsset>();
			json["vertex_array"] = vertex_array_uri;
			json["vertex_skin"] = vertex_skin_uri;
		} else {
			json["type_name"] = get_type_name<StaticPrimitiveAsset>();
			json["vertex_array"] = vertex_array_uri;
		}
		json["vertex_shader"] = vertex_shader;
		json["material"] = material_uri;
		json["topology"] = [&] {
			switch (primitive.mode) {
			case gltf2cpp::PrimitiveMode::ePoints: return "points";
			case gltf2cpp::PrimitiveMode::eLines: return "lines";
			case gltf2cpp::PrimitiveMode::eLineStrip: return "line_strip";
			case gltf2cpp::PrimitiveMode::eLineLoop: return "line_loop";
			case gltf2cpp::PrimitiveMode::eTriangleStrip: return "triangle_strip";
			case gltf2cpp::PrimitiveMode::eTriangleFan: return "triangle_fan";
			default:
			case gltf2cpp::PrimitiveMode::eTriangles: return "triangle_list";
			}
		}();

		return try_export_json(json, std::move(dst_uri), "MeshPrimitive");
	}

	auto import_material(ImportInfo<gltf2cpp::Material> info, ShaderMapping const& shader_mapping) -> std::string {
		if (!should_import(info.dst_uri)) { return std::move(info.dst_uri); }

		auto const fragment_shader = shader_mapping.get_fragment_shader(m_shader);
		if (fragment_shader.empty()) {
			m_log.error("cannot import Material: '{}', fragment shader not provided", info.dst_uri);
			return {};
		}

		auto json = dj::Json{};
		json["type_name"] = get_type_name<LitMaterialAsset>();
		json["material_type"] = material::Lit::type_name_v;
		json["name"] = info.name;
		auto const set_texture = [&](std::optional<gltf2cpp::TextureInfo> const& info, std::string_view const key, bool const mip_map, bool const linear) {
			if (info) {
				auto const texture_uri = import_texture(info->texture, mip_map, linear);
				if (texture_uri.empty()) { return false; }
				json[key] = texture_uri;
			}
			return true;
		};

		auto const& in = *info.asset;
		if (!set_texture(in.pbr.base_color_texture, "base_colour", true, false)) { return {}; }
		if (!set_texture(in.pbr.metallic_roughness_texture, "metallic_roughness", false, true)) { return {}; }
		if (!set_texture(in.emissive_texture, "emissive", false, false)) { return {}; }
		if (in.normal_texture) {
			if (!shader_mapping.is_normal_mapping_enabled()) {
				m_log.warn("ignoring normal map for Material '{}'", info.dst_uri);
			} else {
				if (!set_texture(in.normal_texture->info, "normal", false, true)) { return {}; }
			}
		}

		json["albedo"] = colour::to_hex_string(to_rgba(in.pbr.base_color_factor));
		json["emissive_factor"] = colour::to_hex_string(to_rgba(in.emissive_factor));
		json["metallic"] = in.pbr.metallic_factor;
		json["roughness"] = in.pbr.roughness_factor;
		json["alpha_cutoff"] = in.alpha_cutoff;
		json["is_transparent"] = dj::Boolean{.value = in.alpha_mode == gltf2cpp::AlphaMode::eBlend};
		json["fragment_shader"] = fragment_shader;

		return try_export_json(json, std::move(info.dst_uri), "Material");
	}

	auto import_animation_channels(gltf2cpp::Animation const& animation, std::string_view const anim_name) -> std::string {
		auto dst_uri = get_uri(std::format("{}.channels.bin", anim_name), "skeletons");
		if (!should_import(dst_uri)) { return dst_uri; }

		auto channels = std::vector<AnimationChannel>{};
		channels.reserve(animation.channels.size());
		for (auto const& in_channel : animation.channels) {
			auto out_channel = export_animation_channel(animation, in_channel, dst_uri);
			if (out_channel.sampler.is_empty()) { continue; }
			channels.push_back(std::move(out_channel));
		}

		if (channels.empty()) {
			m_log.error("no Animation Channels to import: '{}'", dst_uri);
			return {};
		}

		auto bytes = std::vector<std::byte>{};
		to_bytes(bytes, channels);
		return try_export_bin(bytes, std::move(dst_uri), "AnimationChannels");
	}

	auto export_animation_channel(gltf2cpp::Animation const& animation, gltf2cpp::Animation::Channel const& channel,
								  std::string_view const dst_uri) -> AnimationChannel {
		if (!channel.target.node) {
			m_log.error("animations without a target node are not supported: '{}'", dst_uri);
			return {};
		}

		auto const& sampler = animation.samplers.at(channel.sampler);
		auto lerp_type = LerpType{};
		switch (sampler.interpolation) {
		case gltf2cpp::Interpolation::eCubicSpline: {
			m_log.error("only linear and step interpolations are supported: '{}'", dst_uri);
			return {};
		}
		case gltf2cpp::Interpolation::eStep: lerp_type = LerpType::eStep; break;
		case gltf2cpp::Interpolation::eLinear: lerp_type = LerpType::eLinear; break;
		}

		auto const& output = m_root->accessors.at(sampler.output);
		switch (output.type) {
		case gltf2cpp::Accessor::Type::eVec3:
		case gltf2cpp::Accessor::Type::eVec4: break;
		default: {
			m_log.error("only vec3 and vec4 sampler data is supported: '{}'", dst_uri);
			return {};
		}
		}

		auto const& input = m_root->accessors.at(sampler.input);
		auto const timestamps = std::get<gltf2cpp::Accessor::Float>(input.data).span();

		auto transform_sampler = TransformSampler{};
		auto copy_keyframes = [&](auto& out, auto const& in) {
			out.reserve(in.size());
			for (auto const& [timestamp, in_t] : std::ranges::zip_view(timestamps, in)) {
				auto& keyframe = out.emplace_back();
				std::memcpy(&keyframe.output, in_t.data(), sizeof(keyframe.output));
				keyframe.timestamp = Seconds{timestamp};
			}
		};
		auto set_sampler = [&](auto sampler, auto keyframes) {
			sampler.set_keyframes(std::move(keyframes));
			sampler.lerp_type = lerp_type;
			transform_sampler.sampler = std::move(sampler);
		};

		switch (channel.target.path) {
		case gltf2cpp::Animation::Path::eRotation: {
			auto keyframes = std::vector<AnimationKeyframe<glm::quat>>{};
			copy_keyframes(keyframes, output.to_vec<4>());
			set_sampler(TransformSampler::Rotator{}, std::move(keyframes));
			break;
		}
		case gltf2cpp::Animation::Path::eTranslation:
		case gltf2cpp::Animation::Path::eScale: {
			auto keyframes = std::vector<AnimationKeyframe<glm::vec3>>{};
			copy_keyframes(keyframes, output.to_vec<3>());
			if (channel.target.path == gltf2cpp::Animation::Path::eScale) {
				set_sampler(TransformSampler::Scaler{}, std::move(keyframes));
			} else {
				set_sampler(TransformSampler::Translator{}, std::move(keyframes));
			}
			break;
		}
		default: {
			m_log.error("only translation, rotation, and scale animation paths are supported: '{}'", dst_uri);
			return {};
		}
		}

		return AnimationChannel{.sampler = std::move(transform_sampler), .target = to_import_index(*channel.target.node)};
	}

	auto find_skin_for(std::size_t const mesh_index) const -> std::optional<std::size_t> {
		for (auto const& node : m_root->nodes) {
			if (!node.mesh || *node.mesh != mesh_index || !node.skin) { continue; }
			return *node.skin;
		}
		return {};
	}

	void add_node_and_children(ImportIndex const index, dj::Json& out_array) {
		auto const& node = m_root->nodes.at(to_size_t(index));
		out_array.push_back(import_node(index));
		for (auto const child_index : node.children) { add_node_and_children(to_import_index(child_index), out_array); }
	}

	auto export_camera(std::size_t const index) -> dj::Json {
		if (!ensure_root() || !check_index(m_root->cameras, index)) { return {}; }
		// check_index

		auto const& camera = m_root->cameras.at(index);
		auto ret = dj::Json{};
		ret["name"] = get_name(camera.name, std::format("camera_{}", index));
		auto const visitor = Visitor{
			[&ret](gltf2cpp::Camera::Perspective const& p) {
				ret["type"] = "perspective";
				ret["y_fov"] = p.yfov;
				ret["z_near"] = p.znear;
				if (p.zfar) { ret["z_far"] = *p.zfar; }
			},
			[&ret](gltf2cpp::Camera::Orthographic const& o) {
				ret["type"] = "orthographic";
				ret["z_near"] = o.znear;
				ret["z_far"] = o.zfar;
			},
		};
		std::visit(visitor, camera.payload);
		return ret;
	}

	auto get_bytes(std::string_view const uri) -> std::span<std::byte const> {
		auto it = m_bytes_map.find(uri);
		if (it == m_bytes_map.end()) {
			auto const [i, _] = m_bytes_map.insert_or_assign(std::string{uri}, m_vfs.load_bytes(uri));
			it = i;
		}
		return it->second;
	}

	void build_import_asset() {
		m_import_asset = {};
		m_import_asset.nodes.reserve(m_root->nodes.size());
		for (auto const [index, in] : std::ranges::enumerate_view(m_root->nodes)) {
			auto out = ImportNode{.index = to_import_index(index)};
			out.name = get_node_name(in);
			if (in.mesh) { out.mesh = to_import_index(*in.mesh); }
			if (in.skin) { out.skeleton = to_import_index(*in.skin); }
			if (in.camera) { out.camera = to_import_index(*in.camera); }
			if (in.parent) { out.parent = to_import_index(*in.parent); }
			for (auto const child : in.children) { out.children.push_back(to_import_index(child)); }
			m_import_asset.nodes.push_back(std::move(out));
		}
		m_import_asset.scenes.reserve(m_root->scenes.size());
		for (auto const& [index, scene] : std::ranges::enumerate_view(m_root->scenes)) {
			auto out = ImportScene{.index = to_import_index(index)};
			out.root_nodes.reserve(scene.root_nodes.size());
			for (auto const index : scene.root_nodes) { out.root_nodes.push_back(to_import_index(index)); }
			m_import_asset.scenes.push_back(std::move(out));
		}
	}

	auto ensure_root() -> bool {
		if (!m_root) {
			auto const parser = gltf2cpp::Parser{m_json};
			m_root = parser.parse([this](std::string_view const uri) { return get_bytes(uri); });
			build_import_asset();
		}

		return true;
	}

	template <typename T>
	auto check_index(T const& arr, std::size_t const index) const -> bool {
		if (index >= arr.size()) {
			m_log.warn("out of bounds index, ignoring: '{}'", index);
			return false;
		}
		return true;
	}

	static auto get_name(std::string_view const in, std::string fallback) -> std::string {
		if (in.empty() || in == "(Unnamed)") { return fallback; }
		return std::string{in};
	}

	auto get_uri(std::string_view const in, std::string_view const subdir) const -> std::string {
		return (fs::path{m_import_root_dir} / m_uri_prefix / subdir / in).generic_string();
	}

	auto get_path(std::string_view const uri) const -> std::string { return (fs::path{m_mount_point} / uri).generic_string(); }

	auto should_import(std::string_view const dst_uri) const -> bool {
		if (m_force_import) { return !m_imported.contains(dst_uri); }
		auto const path = fs::path{m_mount_point} / dst_uri;
		if (fs::exists(path)) {
			m_log.info("skipping import of existing asset: '{}'", dst_uri);
			return false;
		}
		return true;
	}

	auto success(std::string uri, std::string_view type) -> std::string {
		m_log.info("imported {}: '{}'", type, uri);
		m_imported.insert(uri);
		return uri;
	}

	[[nodiscard]] static auto get_node_name(gltf2cpp::Node const& node) -> std::string {
		if (!node.name.empty()) { return node.name; }
		return std::format("node_{}", node.self);
	}

	Logger m_log{"Importer"};

	std::string m_mount_point{};
	Shader m_shader{};
	std::string m_import_root_dir{};
	bool m_force_import{false};

	dj::Json m_json{};
	std::string m_uri_prefix{};
	std::unordered_map<std::string, std::vector<std::byte>, StringHash, std::equal_to<>> m_bytes_map{};
	std::unordered_set<std::string, StringHash, std::equal_to<>> m_imported{};
	VfsFiles m_vfs{};

	std::optional<gltf2cpp::Root> m_root{};
	ImportAsset m_import_asset{};

	friend class Builder;
};

void Importer::Deleter::operator()(Impl* ptr) const noexcept { std::default_delete<Impl>{}(ptr); }

auto Importer::get_import_asset() const -> ImportAsset const& {
	static auto const blank_v = ImportAsset{};
	if (!m_impl) { return blank_v; }
	return m_impl->get_import_asset();
}

auto Importer::get_mesh_name(ImportIndex index) const -> std::string_view {
	if (!m_impl) { return {}; }
	return m_impl->get_mesh_name(index);
}

auto Importer::get_skeleton_name(ImportIndex index) const -> std::string_view {
	if (!m_impl) { return {}; }
	return m_impl->get_skeleton_name(index);
}

auto Importer::import_node(ImportIndex const index) -> dj::Json {
	if (!m_impl) { return {}; }
	return m_impl->import_node(index);
}

auto Importer::import_scene(ImportIndex const index) -> std::string {
	if (!m_impl) { return {}; }
	return m_impl->import_scene(index);
}

Importer::Builder::Builder(std::string_view mount_point, Shader shader) noexcept(false)
	: m_mount_point(fs::absolute(mount_point).generic_string()), m_shader(shader) {
	if (!set_mount_point(mount_point)) { throw Error{"Importer: failed to set import root directory"}; }
	if (m_shader.lit_vertex_uri.empty() || m_shader.lit_fragment_uri.empty()) { throw Error{"Importer: missing required lit shader(s)"}; }
	if (m_shader.skinned_vertex_uri.empty()) { m_log.warn("missing skin vertex shader, cannot import skinned mesh primitives"); }
	if (m_shader.tbn_vertex_uri.empty() || m_shader.tbn_fragment_uri.empty()) {
		m_log.warn("missing TBN shader(s), using lit shaders for normal mapped primitives / materials");
	}
}

auto Importer::Builder::set_mount_point(std::string_view const directory) -> bool {
	auto mount_point = fs::absolute(directory);
	auto const exists = fs::exists(mount_point);
	if (exists && !fs::is_directory(mount_point)) {
		m_log.error("import root is not a directory: '{}'", directory);
		return false;
	}
	if (!exists) {
		if (!do_create_directories(mount_point)) {
			m_log.error("failed to create directory for import root: '{}'", directory);
			return false;
		}
		m_log.info("created import root directory: '{}'", directory);
	}

	m_mount_point = mount_point.generic_string();
	m_log.info("import root set to: '{}'", m_mount_point);
	return true;
}

auto Importer::Builder::build(char const* path) const noexcept(false) -> Importer {
	auto ret = Importer{};
	ret.m_impl.reset(new Impl{}); // NOLINT(cppcoreguidelines-owning-memory)
	ret.m_impl->m_mount_point = m_mount_point;
	ret.m_impl->m_shader = m_shader;
	ret.m_impl->m_import_root_dir = import_root_dir;
	ret.m_impl->m_force_import = force_import;

	if (!load_gltf(path, ret)) { throw Error{"Importer: failed to load GLTF"}; }
	return ret;
}

auto Importer::Builder::load_gltf(char const* path, Importer& out) const -> bool {
	auto json = dj::Json::from_file(path);
	if (!json) {
		m_log.error("failed to load json from: '{}'", path);
		return false;
	}

	auto const filename = fs::path{path}.filename();
	if (!do_create_directories(fs::path{m_mount_point} / import_root_dir / filename.stem())) {
		m_log.error("failed to create asset subdirectory");
		return false;
	}

	out.m_impl->m_json = std::move(json);
	out.m_impl->m_uri_prefix = filename.stem().string();
	auto const gltf_dir = fs::path{path}.parent_path();
	out.m_impl->m_vfs = VfsFiles{gltf_dir.generic_string()};

	m_log.info("loaded GLTF '{}', URI prefix set to '{}'", filename.generic_string(), out.m_impl->m_uri_prefix);

	return true;
}
} // namespace levk
