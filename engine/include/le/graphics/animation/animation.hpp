#pragma once
#include <le/core/transform.hpp>
#include <le/graphics/animation/interpolator.hpp>
#include <le/node/node_tree.hpp>
#include <variant>

namespace le::graphics {
struct Animation {
	struct Translate : Interpolator<glm::vec3> {};
	struct Rotate : Interpolator<glm::quat> {};
	struct Scale : Interpolator<glm::vec3> {};

	struct Channel {
		Id<Node> joint_id;
		std::variant<Translate, Rotate, Scale> storage{};

		auto update(NodeLocator node_locator, Duration time) const -> void;
		[[nodiscard]] auto duration() const -> Duration;
	};

	std::vector<Channel> channels{};

	[[nodiscard]] auto duration() const -> Duration;
	auto update(NodeLocator node_locator, Duration time) const -> void;
};
} // namespace le::graphics
