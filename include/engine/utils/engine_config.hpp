#pragma once
#include <core/io/converters.hpp>
#include <optional>

namespace le {
namespace utils {
struct EngineConfig {
	struct Window {
		std::optional<glm::uvec2> position;
		glm::uvec2 size{};
		bool maximized{};
	};

	Window win;
};
} // namespace utils

template <>
struct io::Jsonify<utils::EngineConfig> : JsonHelper {
	dj::json operator()(utils::EngineConfig const& config) const;
	utils::EngineConfig operator()(dj::json const& json) const;
};
} // namespace le
