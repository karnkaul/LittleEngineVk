#include <ktl/debug_trap.hpp>
#include <levk/engine/assets/asset_converters.hpp>

namespace le::io {
namespace {
graphics::RGBA parseRGBA(dj::json const& json, graphics::RGBA fallback) {
	if (json.is_string()) {
		if (auto str = json.as<std::string_view>(); !str.empty() && str[0] == '#') { return Colour(str); }
	} else if (json.contains("colour")) {
		if (auto str = json.get_as<std::string_view>("colour"); !str.empty() && str[0] == '#') {
			graphics::RGBA ret;
			ret.colour = Colour(str);
			ret.type = json.get_as<std::string_view>("type") == "absolute" ? graphics::RGBA::Type::eAbsolute : graphics::RGBA::Type::eIntensity;
			return ret;
		}
	}
	return fallback;
}
} // namespace

dj::json Jsonify<RenderFlags>::operator()(RenderFlags const& flags) const {
	dj::json ret;
	if (flags == graphics::pflags_all) {
		ret.push_back(to<std::string>("all"));
		return ret;
	}
	if (flags.test(RenderFlag::eDepthTest)) { ret.push_back(to<std::string>("depth_test")); }
	if (flags.test(RenderFlag::eDepthWrite)) { ret.push_back(to<std::string>("depth_write")); }
	if (flags.test(RenderFlag::eAlphaBlend)) { ret.push_back(to<std::string>("alpha_blend")); }
	if (flags.test(RenderFlag::eWireframe)) { ret.push_back(to<std::string>("wireframe")); }
	return ret;
}

RenderFlags Jsonify<RenderFlags>::operator()(dj::json const& json) const {
	RenderFlags ret;
	if (json.is_array()) {
		for (auto const flag : json.as<std::vector<std::string_view>>()) {
			if (flag == "all") {
				ret = graphics::pflags_all;
				return ret;
			}
			if (flag == "depth_test") {
				ret.set(RenderFlag::eDepthTest);
			} else if (flag == "depth_write") {
				ret.set(RenderFlag::eDepthWrite);
			} else if (flag == "alpha_blend") {
				ret.set(RenderFlag::eAlphaBlend);
			} else if (flag == "wireframe") {
				ret.set(RenderFlag::eWireframe);
			}
		}
	}
	return ret;
}

dj::json Jsonify<RenderLayer>::operator()(RenderLayer const& layer) const {
	dj::json ret;
	insert(ret, "mode", polygonModes[layer.mode], "topology", topologies[layer.topology], "line_width", layer.lineWidth, "order", layer.order);
	ret.insert("flags", to(layer.flags));
	return ret;
}

RenderLayer Jsonify<RenderLayer>::operator()(dj::json const& json) const {
	RenderLayer ret;
	if (auto mode = json.get_as<std::string_view>("mode"); !mode.empty()) { ret.mode = polygonModes[mode]; }
	if (auto top = json.get_as<std::string_view>("topology"); !top.empty()) { ret.topology = topologies[top]; }
	ret.flags = to<RenderFlags>(json.get("flags"));
	ret.lineWidth = json.get_as<f32>("line_width", ret.lineWidth);
	ret.order = json.get_as<s64>("order");
	return ret;
}

Material Jsonify<Material>::operator()(dj::json const& json) const {
	Material mat;
	mat.Ka = parseRGBA(json.get("Ka"), mat.Ka);
	mat.Kd = parseRGBA(json.get("Kd"), mat.Kd);
	mat.Ks = parseRGBA(json.get("Ks"), mat.Ks);
	mat.Tf = parseRGBA(json.get("Tf"), mat.Tf);
	mat.Ns = json.get_as<f32>("Ns", mat.Ns);
	mat.d = json.get_as<f32>("d", mat.d);
	mat.illum = json.get_as<int>("illum", mat.illum);
	mat.map_Kd = Hash(json.get_as<std::string_view>("map_Kd"));
	mat.map_Ks = Hash(json.get_as<std::string_view>("map_Ks"));
	mat.map_d = Hash(json.get_as<std::string_view>("map_d"));
	mat.map_Bump = Hash(json.get_as<std::string_view>("map_bump"));
	return mat;
}
} // namespace le::io
