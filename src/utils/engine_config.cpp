#include <engine/utils/engine_config.hpp>

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
	if (auto win = json.find("window")) {
		if (win->contains("position")) { set(*ret.win.position, win->get("position")); }
		set(ret.win.size, win->get("size"));
		set(ret.win.maximized, win->get("maximized"));
	}
	return ret;
}
} // namespace le
