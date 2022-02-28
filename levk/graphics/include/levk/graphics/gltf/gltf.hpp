#pragma once
#include <functional>
#include <optional>
#include <unordered_map>
#include "data_builder.hpp"

namespace dj {
class json;
}

namespace le::gltf {
enum class error_t {
	unknown_error,
	resource_not_found,
	missing_required_property,
	mismatched_size,
	invalid_data_uri,
	invalid_accessor,
	out_of_range,
	unsupported,
	none,
};

struct version_t {
	int major{};
	int minor{};
	int patch{};

	constexpr auto operator<=>(version_t const&) const = default;

	static version_t make(std::string_view version) noexcept;
};

using get_bytes_t = std::function<std::vector<std::byte>(std::string_view)>;

constexpr tvec3<float> scale_v = {{1.0f, 1.0f, 1.0f}};
constexpr quat_t orient_v = {{0.0f, 0.0f, 0.0f, 1.0f}};

struct buffer_t {
	std::string name{};

	std::vector<std::byte> storage{};
};

struct buffer_view_t {
	enum class type_t { none, array, element_array };

	std::string name{};

	buffer_span buffer{};
	std::size_t byte_stride{};
	type_t target{};
};

struct accessor_t {
	enum class type_t { scalar, vec2, vec3, vec4, mat2, mat3, mat4 };
	enum class ctype_t { sbyte, ubyte, sshort, ushort, uint, floating };

	std::string name{};

	type_t type{};
	ctype_t ctype{};
	std::size_t count{};

	buffer_t inline_buffer{};
	std::optional<std::size_t> buffer_view_index{};
	std::size_t byte_offset{};
	bool normalized{};
};

struct resources_t {
	std::vector<buffer_t> buffers{};
	std::vector<buffer_view_t> buffer_views{};
	std::vector<accessor_t> accessors{};
};

struct primitive_t {
	enum class mode_t { points = 0, lines, line_loop, line_strip, triangles, triangle_strip, triangle_fan };

	std::unordered_map<std::string, std::size_t> custom_attributes{};
	std::vector<tvec3<float>> positions{};
	std::vector<tvec3<float>> normals{};
	std::vector<tvec2<float>> texcoords{};
	std::vector<tvec4<float>> colours{};
	std::vector<std::uint32_t> indices{};
	std::optional<std::size_t> material_index{};
	mode_t mode = mode_t::triangles;
};

struct sampler_t {
	enum class min_filter_t { nearest, linear, nearest_mipmap_nearest, linear_mipmap_nearest, nearest_mipmap_linear, linear_mipmap_linear };
	enum class mag_filter_t { nearest, linear };
	enum class wrap_t { clamp_to_edge, mirrored_repeat, repeat };

	std::string name{};

	std::optional<mag_filter_t> mag{};
	std::optional<min_filter_t> min{};
	wrap_t wrapt = wrap_t::repeat;
	wrap_t wraps = wrap_t::repeat;
};

struct image_t {
	enum class type_t { jpeg, png };

	std::string name{};

	std::optional<std::size_t> buffer_view_index;
	std::vector<std::byte> bytes;
	type_t type{};
};

struct texture_t {
	std::string name{};

	std::optional<std::size_t> source{};
	std::optional<std::size_t> sampler{};
};

struct texture_info_t {
	std::size_t index{};
	std::size_t tex_coord{};
};

struct pbr_metallic_roughness_t {
	tvec4<float> base_colour_factor = {{1.0f, 1.0f, 1.0f, 1.0f}};
	std::optional<texture_info_t> base_colour_texture{};
	std::optional<texture_info_t> metallic_roughness_texture{};
	float metallic_factor = 1.0f;
	float roughness_factor = 1.0f;
};

struct normal_texture_info_t : texture_info_t {
	float scale = 1.0f;
};

struct occlusion_texture_info_t : texture_info_t {
	float strength = 1.0f;
};

struct material_t {
	enum class alpha_mode_t { opaque, mask, blend };

	std::string name;

	pbr_metallic_roughness_t pbr_metallic_roughness{};
	normal_texture_info_t normal_texture{};
	occlusion_texture_info_t occlusion_texture{};
	texture_info_t emissive_texture{};
	tvec3<float> emissive_factor{};
	alpha_mode_t alpha_mode = alpha_mode_t::opaque;
	float alpha_cutoff = 0.5f;
	bool double_sided = false;
};

struct mesh_t {
	std::string name{};

	std::vector<primitive_t> primitives{};
};

struct camera_perspective_t {
	float yfov{};
	float znear{};

	std::optional<float> zfar{};
	std::optional<float> aspect_ratio{};
};

struct camera_orthographic_t {
	float xmag{};
	float ymag{};
	float zfar{};
	float znear{};
};

struct camera_t {
	enum class type_t { perspective, orthographic };

	std::string name{};

	type_t type{};

	camera_perspective_t perspective;
	camera_orthographic_t orthographic;
};

struct node_t {
	std::string name{};

	tvec3<float> translation{};
	tvec3<float> scale = scale_v;
	quat_t rotation = orient_v;
	std::optional<std::size_t> mesh_index{};
	std::optional<std::size_t> camera_index{};
	std::vector<std::size_t> child_indices{};
};

struct scene_t {
	std::string name{};

	std::vector<std::size_t> node_indices{};
};

struct result_t;

struct asset_t {
	resources_t resources{};
	version_t version{};

	version_t min_version{};
	std::string copyright{};
	std::string generator{};
	std::vector<image_t> images{};
	std::vector<sampler_t> samplers{};
	std::vector<texture_t> textures{};
	std::vector<material_t> materials{};
	std::vector<camera_t> cameras{};
	std::vector<scene_t> scenes{};
	std::vector<node_t> nodes{};
	std::vector<mesh_t> meshes{};
	std::size_t scene{};

	static result_t parse(char const* json_uri, get_bytes_t get_bytes = {});
};

struct result_t {
	asset_t asset{};
	error_t error{};

	bool ok() const noexcept { return error == error_t::none; }
	explicit operator bool() const noexcept { return ok(); }
};
} // namespace le::gltf
