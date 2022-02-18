#include <dumb_json/json.hpp>
#include <levk/graphics/gltf/gltf.hpp>
#include <optional>
#include <sstream>

namespace le::gltf {
namespace {
std::size_t bytes_size(accessor_t const& accessor) {
	using ctype_t = accessor_t::ctype_t;
	using type_t = accessor_t::type_t;
	std::size_t csize{};
	switch (accessor.ctype) {
	case ctype_t::sbyte:
	case ctype_t::ubyte: csize = 1U; break;
	case ctype_t::sshort:
	case ctype_t::ushort: csize = 2U; break;
	case ctype_t::uint:
	case ctype_t::floating: csize = 4U; break;
	}
	switch (accessor.type) {
	case type_t::scalar: break;
	case type_t::vec2: csize *= 2; break;
	case type_t::vec3: csize *= 3; break;
	case type_t::vec4: csize *= 4; break;
	case type_t::mat2: csize *= 2 * 2; break;
	case type_t::mat3: csize *= 3 * 3; break;
	case type_t::mat4: csize *= 4 * 4; break;
	}
	return csize * accessor.count;
}

constexpr std::optional<buffer_view_t::type_t> buffer_type(int type) {
	switch (type) {
	case 34962: return buffer_view_t::type_t::array;
	case 34963: return buffer_view_t::type_t::element_array;
	default: break;
	}
	return {};
}

constexpr std::optional<accessor_t::ctype_t> accessor_ctype(int ctype) {
	switch (ctype) {
	case 5120: return accessor_t::ctype_t::sbyte;
	case 5121: return accessor_t::ctype_t::ubyte;
	case 5122: return accessor_t::ctype_t::sshort;
	case 5123: return accessor_t::ctype_t::ushort;
	case 5125: return accessor_t::ctype_t::uint;
	case 5126: return accessor_t::ctype_t::floating;
	default: break;
	}
	return {};
}

constexpr std::optional<accessor_t::type_t> accessor_type(std::string_view ctype) {
	if (ctype == "SCALAR") { return accessor_t::type_t::scalar; }
	if (ctype == "VEC2") { return accessor_t::type_t::vec2; }
	if (ctype == "VEC3") { return accessor_t::type_t::vec3; }
	if (ctype == "VEC4") { return accessor_t::type_t::vec4; }
	if (ctype == "MAT2") { return accessor_t::type_t::mat2; }
	if (ctype == "MAT3") { return accessor_t::type_t::mat3; }
	if (ctype == "MAT4") { return accessor_t::type_t::mat4; }
	return {};
}

constexpr std::optional<primitive_t::mode_t> primitive_mode(int mode) {
	switch (mode) {
	case 0: return primitive_t::mode_t::points;
	case 1: return primitive_t::mode_t::lines;
	case 2: return primitive_t::mode_t::line_loop;
	case 3: return primitive_t::mode_t::line_strip;
	case 4: return primitive_t::mode_t::triangles;
	case 5: return primitive_t::mode_t::triangle_strip;
	case 6: return primitive_t::mode_t::triangle_fan;
	}
	return {};
}

constexpr std::optional<camera_t::type_t> camera_type(std::string_view type) {
	if (type == "perspective") { return camera_t::type_t::perspective; }
	if (type == "orthographic") { return camera_t::type_t::orthographic; }
	return {};
}

error_t make_indices(std::span<buffer_view_t const> buffers, accessor_t const& accessor, std::vector<std::uint32_t>& out) {
	if (accessor.type != accessor_t::type_t::scalar) { return error_t::invalid_accessor; }
	if (!accessor.buffer_view_index) { return error_t::invalid_accessor; }
	auto const& buffer = buffers[*accessor.buffer_view_index];
	switch (accessor.ctype) {
	case accessor_t::ctype_t::uint: out = build_data<std::uint32_t>(buffer.buffer, buffer.byte_stride); break;
	case accessor_t::ctype_t::ushort: out = build_data<std::uint32_t, std::uint16_t>(buffer.buffer, buffer.byte_stride); break;
	default: return error_t::invalid_accessor;
	}
	return error_t::none;
}

template <typename T, std::size_t N>
void fill_array(T (&out)[N], dj::json const& json, char const* key) {
	if (auto vec = json.get_as<std::vector<T>>(key); vec.size() == N) {
		for (std::size_t i = 0; i < N; ++i) { out[i] = vec[i]; }
	}
}

struct parser {
	asset_t& out;
	get_uri_bytes_t const& get_uri_bytes;

	error_t parse_asset(dj::json const& root);
	error_t parse_resources(dj::json const& root);
	error_t parse_meshes(dj::json const& root);
	error_t parse_cameras(dj::json const& root);
	error_t parse_nodes(dj::json const& root);
	error_t parse_scenes(dj::json const& root);

	error_t parse_buffer(std::string_view uri, buffer_storage_t& out_storage) const;
	error_t parse_buffers(dj::json const& root);
	error_t parse_buffer_views(dj::json const& root);
	error_t parse_accessors(dj::json const& root);
	error_t parse_attributes(dj::json const& primitive, primitive_t& out_prim) const;

	error_t operator()(dj::json const& json) {
		auto result = parse_asset(json);
		if (result != error_t::none) { return result; }
		if (result = parse_resources(json); result != error_t::none) { return result; }
		if (result = parse_meshes(json); result != error_t::none) { return result; }
		if (result = parse_cameras(json); result != error_t::none) { return result; }
		if (result = parse_nodes(json); result != error_t::none) { return result; }
		if (result = parse_scenes(json); result != error_t::none) { return result; }
		return result;
	}
};

error_t parser::parse_asset(dj::json const& root) {
	auto asset = root.find("asset");
	if (!asset) { return error_t::missing_required_property; }
	auto version = asset->find_as<std::string_view>("version");
	if (!version) { return error_t::missing_required_property; }
	out.version = version_t::make(*version);
	out.min_version = version_t::make(asset->get_as<std::string_view>("minVersion"));
	out.copyright = asset->get_as<std::string>("copyright");
	out.generator = asset->get_as<std::string>("generator");
	return error_t::none;
}

error_t parser::parse_resources(dj::json const& root) {
	auto res = parse_buffers(root);
	if (res != error_t::none) { return res; }
	if (res = parse_buffer_views(root); res != error_t::none) { return res; }
	if (res = parse_accessors(root); res != error_t::none) { return res; }
	return error_t::none;
}

error_t parser::parse_meshes(dj::json const& root) {
	for (auto const& mesh : root.get_as<dj::vec_t>("meshes")) {
		mesh_t msh;
		for (auto const& primitive : mesh->get_as<dj::vec_t>("primitives")) {
			primitive_t prim;
			if (auto res = parse_attributes(*primitive, prim); res != error_t::none) { return res; }
			if (auto mode = primitive->find_as<int>("mode")) {
				auto omode = primitive_mode(*mode);
				if (!omode) { return error_t::out_of_range; }
				prim.mode = *omode;
			}
			if (auto indices = primitive->find_as<std::size_t>("indices")) {
				if (*indices >= out.resources.accessors.size()) { return error_t::out_of_range; }
				std::vector<std::uint32_t> indices_vec;
				auto res = make_indices(out.resources.buffer_views, out.resources.accessors[*indices], indices_vec);
				if (res != error_t::none) { return res; }
				prim.indices = std::move(indices_vec);
			}
			if (auto mtl_index = primitive->find_as<std::size_t>("material")) { prim.material_index = *mtl_index; }
			msh.primitives.push_back(std::move(prim));
		}
		if (msh.primitives.empty()) { return error_t::missing_required_property; }
		msh.name = mesh->get_as<std::string>("name");
		out.meshes.push_back(std::move(msh));
	}
	return error_t::none;
}

error_t parser::parse_cameras(dj::json const& root) {
	for (auto const& camera : root.get_as<dj::vec_t>("cameras")) {
		auto type = camera_type(camera->get_as<std::string_view>("type"));
		if (!type) { return error_t::missing_required_property; }
		camera_t cam;
		cam.type = *type;
		switch (cam.type) {
		case camera_t::type_t::orthographic: {
			auto ortho = camera->find("orthographic");
			if (!ortho) { return error_t::missing_required_property; }
			auto xmag = ortho->find_as<float>("xmag");
			auto ymag = ortho->find_as<float>("ymag");
			auto zfar = ortho->find_as<float>("zfar");
			auto znear = ortho->find_as<float>("znear");
			if (!xmag || !ymag || !zfar || !znear) { return error_t::missing_required_property; }
			cam.orthographic.xmag = *xmag;
			cam.orthographic.ymag = *ymag;
			cam.orthographic.zfar = *zfar;
			cam.orthographic.znear = *znear;
			break;
		}
		case camera_t::type_t::perspective: {
			auto persp = camera->find("perspective");
			if (!persp) { return error_t::missing_required_property; }
			auto yfov = persp->find_as<float>("yfov");
			auto znear = persp->find_as<float>("znear");
			if (!yfov || !znear) { return error_t::missing_required_property; }
			cam.perspective.yfov = *yfov;
			cam.perspective.znear = *znear;
			if (auto zfar = persp->find_as<float>("zfar")) { cam.perspective.zfar = *zfar; }
			if (auto ar = persp->find_as<float>("aspectRatio")) { cam.perspective.aspect_ratio = *ar; }
			break;
		}
		}
		cam.name = camera->get_as<std::string>("name");
		out.cameras.push_back(std::move(cam));
	}
	return error_t::none;
}

error_t parser::parse_nodes(dj::json const& root) {
	for (auto const& node : root.get_as<dj::vec_t>("nodes")) {
		node_t nd;
		if (auto cam = node->find_as<std::size_t>("camera")) {
			if (*cam >= out.cameras.size()) { return error_t::out_of_range; }
			nd.camera_index = *cam;
		}
		if (auto mesh = node->find_as<std::size_t>("mesh")) {
			if (*mesh >= out.meshes.size()) { return error_t::out_of_range; }
			nd.mesh_index = *mesh;
		}
		nd.name = node->get_as<std::string>("name");
		fill_array(nd.rotation.data, *node, "rotation");
		fill_array(nd.translation.data, *node, "translation");
		fill_array(nd.scale.data, *node, "scale");
		nd.child_indices = node->get_as<decltype(nd.child_indices)>("children");
		out.nodes.push_back(std::move(nd));
	}
	return error_t::none;
}

error_t parser::parse_scenes(dj::json const& root) {
	out.scene = root.get_as<std::size_t>("scene");
	for (auto const& scene : root.get_as<dj::vec_t>("scenes")) {
		scene_t scn;
		scn.name = scene->get_as<std::string>("name");
		scn.node_indices = scene->get_as<decltype(scn.node_indices)>("nodes");
		out.scenes.push_back(std::move(scn));
	}
	return error_t::none;
}

error_t parser::parse_buffers(dj::json const& root) {
	for (auto const& buffer : root.get_as<dj::vec_t>("buffers")) {
		auto length = buffer->find_as<std::size_t>("byteLength");
		if (!length) { return error_t::missing_required_property; }
		auto uri = buffer->get_as<std::string_view>("uri");
		buffer_t bf;
		if (uri.empty()) { return error_t::unsupported; }
		auto res = parse_buffer(uri, bf.storage);
		if (res != error_t::none) { return res; }
		if (bf.storage.size() != *length) { return error_t::mismatched_size; }
		bf.name = buffer->get_as<std::string>("name");
		out.resources.buffers.push_back(std::move(bf));
	}
	return error_t::none;
}

error_t parser::parse_buffer_views(dj::json const& root) {
	for (auto const& view : root.get_as<dj::vec_t>("bufferViews")) {
		auto length = view->find_as<std::size_t>("byteLength");
		auto buffer = view->find_as<std::size_t>("buffer");
		if (!length || !buffer) { return error_t::missing_required_property; }
		if (*buffer >= out.resources.buffers.size()) { return error_t::out_of_range; }
		auto const offset = view->get_as<std::size_t>("byteOffset");
		if (offset >= *length) { return error_t::out_of_range; }
		buffer_view_t bv;
		bv.buffer = buffer_span(out.resources.buffers[*buffer].storage).subspan(offset, *length);
		if (auto stride = view->find_as<std::size_t>("byteStride")) {
			if (*stride < 4U || *stride > 252U) { return error_t::out_of_range; }
			bv.byte_stride = *stride;
		}
		if (auto target = view->find_as<int>("target")) {
			auto otype = buffer_type(*target);
			if (!otype) { return error_t::out_of_range; }
			bv.target = *otype;
		}
		bv.name = view->get_as<std::string>("name");
		out.resources.buffer_views.push_back(std::move(bv));
	}
	return error_t::none;
}

error_t parser::parse_accessors(dj::json const& root) {
	for (auto const& accessor : root.get_as<dj::vec_t>("accessors")) {
		auto ctype = accessor->find_as<int>("componentType");
		auto count = accessor->find_as<std::size_t>("count");
		auto type = accessor->find_as<std::string_view>("type");
		if (!ctype || !count || !type) { return error_t::missing_required_property; }
		auto octype = accessor_ctype(*ctype);
		auto otype = accessor_type(*type);
		if (!octype || !otype) { return error_t::out_of_range; }
		accessor_t acc;
		if (auto view_index = accessor->find_as<std::size_t>("bufferView")) {
			if (*view_index >= out.resources.buffer_views.size()) { return error_t::out_of_range; }
			acc.buffer_view_index = *view_index;
		} else {
			acc.inline_buffer.storage.resize(bytes_size(acc));
		}
		acc.ctype = *octype;
		acc.type = *otype;
		acc.count = *count;
		acc.byte_offset = accessor->get_as<std::size_t>("byteOffset");
		acc.normalized = accessor->get_as<bool>("normalized");
		acc.name = accessor->get_as<std::string>("name");
		out.resources.accessors.push_back(std::move(acc));
	}
	return error_t::none;
}

error_t parser::parse_buffer(std::string_view uri, buffer_storage_t& out_storage) const {
	if (uri.empty()) { return error_t::missing_required_property; }
	if (uri.size() > 5 && uri.substr(0, 5) == "data:") {
		auto const comma = uri.find(',');
		if (comma == std::string_view::npos) { return error_t::invalid_data_uri; }
		auto const buf_str = uri.substr(comma + 1);
		if (buf_str.empty()) { return error_t::invalid_data_uri; }
		out_storage = base64_decode(buf_str);
		return error_t::none;
	}
	out_storage = get_uri_bytes(uri);
	return error_t::none;
}

error_t parser::parse_attributes(dj::json const& primitive, primitive_t& out_prim) const {
	auto attributes = primitive.find_as<dj::map_t>("attributes");
	if (!attributes) { return error_t::missing_required_property; }
	for (auto& [id, attribute] : *attributes) {
		if (!attribute->is_number()) { return error_t::invalid_accessor; }
		auto accessor_index = attribute->as<std::size_t>();
		if (accessor_index >= out.resources.accessors.size()) { return error_t::out_of_range; }
		auto const& accessor = out.resources.accessors[accessor_index];
		auto const& buffer = out.resources.buffer_views[*accessor.buffer_view_index];
		if (id == "POSITION") {
			if (accessor.type != accessor_t::type_t::vec3 || accessor.ctype != accessor_t::ctype_t::floating) { return error_t::invalid_accessor; }
			out_prim.positions = build_data<tvec3<float>>(buffer.buffer, buffer.byte_stride);
		} else if (id == "NORMAL") {
			if (accessor.type != accessor_t::type_t::vec3 || accessor.ctype != accessor_t::ctype_t::floating) { return error_t::invalid_accessor; }
			out_prim.normals = build_data<tvec3<float>>(buffer.buffer, buffer.byte_stride);
		} else if (id == "TEXCOORD_0") {
			if (accessor.type != accessor_t::type_t::vec2) { return error_t::invalid_accessor; }
			if (accessor.ctype != accessor_t::ctype_t::floating) { return error_t::unsupported; }
			out_prim.texcoords = build_data<tvec2<float>>(buffer.buffer, buffer.byte_stride);
		} else if (id == "COLOR_0") {
			if (accessor.type != accessor_t::type_t::vec4) { return error_t::unsupported; }
			if (accessor.ctype != accessor_t::ctype_t::floating) { return error_t::unsupported; }
			out_prim.colours = build_data<tvec4<float>>(buffer.buffer, buffer.byte_stride);
		} else {
			out_prim.custom_attributes.emplace(std::move(id), accessor_index);
		}
	}
	return error_t::none;
}
} // namespace

version_t version_t::make(std::string_view version) noexcept {
	if (version.empty()) { return {}; }
	std::stringstream str(version.data());
	auto assign = [&str](int& out) {
		if (str >> out) {
			char dot;
			str >> dot;
		}
	};
	version_t ret;
	assign(ret.major);
	assign(ret.minor);
	assign(ret.patch);
	return ret;
}

result_t asset_t::parse(dj::json const& root, get_uri_bytes_t const& get_uri_bytes) {
	if (!get_uri_bytes) { return {{}, error_t::missing_required_property}; }
	result_t ret;
	ret.error = parser{ret.asset, get_uri_bytes}(root);
	return ret;
}
} // namespace le::gltf
