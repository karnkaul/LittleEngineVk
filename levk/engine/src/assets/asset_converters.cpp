#include <levk/engine/assets/asset_converters.hpp>

namespace le::io {
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
		for (auto const& elem : json.as_array()) {
			auto const flag = elem.as_string_view();
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
	insert(ret, "mode", polygonModes[layer.mode], "topology", topologies[layer.topology], "line_width", layer.lineWidth, "order", s64(layer.order));
	ret.insert("flags", to(layer.flags));
	return ret;
}

RenderLayer Jsonify<RenderLayer>::operator()(dj::json const& json) const {
	RenderLayer ret;
	if (auto mode = json["mode"].as_string_view(); !mode.empty()) { ret.mode = polygonModes[mode]; }
	if (auto top = json["topology"].as_string_view(); !top.empty()) { ret.topology = topologies[top]; }
	ret.flags = to<RenderFlags>(json["flags"]);
	ret.lineWidth = json["line_width"].as_number<float>(ret.lineWidth);
	ret.order = RenderOrder{json["order"].as_number<s64>()};
	return ret;
}
} // namespace le::io
