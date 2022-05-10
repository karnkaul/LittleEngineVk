#include <levk/engine/utils/engine_config.hpp>

namespace le {
dj::json io::Jsonify<utils::EngineConfig>::operator()(utils::EngineConfig const& config) const {
	dj::json ret;
	dj::json window = build("size", config.win.size, "maximized", config.win.maximized);
	if (config.win.position) { window.insert("position", to(*config.win.position)); }
	ret.insert("window", std::move(window));
	return ret;
}

utils::EngineConfig io::Jsonify<utils::EngineConfig>::operator()(dj::json const& json) const {
	utils::EngineConfig ret;
	if (auto const& win = json["window"]; win.is_object()) {
		if (win.contains("position")) { ret.win.position = to<glm::vec2>(win["position"]); }
		set(ret.win.size, win["size"]);
		set(ret.win.maximized, win["maximized"]);
	}
	return ret;
}
} // namespace le
