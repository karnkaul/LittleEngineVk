#include <spaced/core/visitor.hpp>
#include <spaced/graphics/animation/animation.hpp>

namespace spaced::graphics {
auto Animation::Channel::duration() const -> Duration {
	return std::visit([](auto const& s) { return s.duration(); }, storage);
}

void Animation::Channel::update(NodeLocator node_locator, Duration time) const {
	auto* joint = node_locator.find(joint_id);
	if (joint == nullptr) { return; }
	auto const visitor = Visitor{
		[time, joint](Animation::Translate const& translate) {
			if (auto const p = translate(time)) { joint->transform.set_position(*p); }
		},
		[time, joint](Animation::Rotate const& rotate) {
			if (auto const o = rotate(time)) { joint->transform.set_orientation(*o); }
		},
		[time, joint](Animation::Scale const& scale) {
			if (auto const s = scale(time)) { joint->transform.set_scale(*s); }
		},
	};
	std::visit(visitor, storage);
}

auto Animation::duration() const -> Duration {
	auto ret = Duration{};
	for (auto const& sampler : channels) { ret = std::max(ret, sampler.duration()); }
	return ret;
}

auto Animation::update(NodeLocator node_locator, Duration time) const -> void {
	for (auto const& channel : channels) { channel.update(node_locator, time); }
}
} // namespace spaced::graphics
