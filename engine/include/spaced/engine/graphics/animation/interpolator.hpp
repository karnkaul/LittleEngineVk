#pragma once
#include <spaced/engine/core/time.hpp>
#include <spaced/engine/transform.hpp>
#include <algorithm>
#include <cassert>
#include <optional>
#include <vector>

namespace spaced::graphics {
constexpr auto lerp(glm::vec3 const& a, glm::vec3 const& b, float const t) -> glm::vec3 {
	return {std::lerp(a.x, b.x, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t)};
}

inline auto lerp(glm::quat const& a, glm::quat const& b, float const t) -> glm::quat { return glm::slerp(a, b, t); }

enum class Interpolation : std::uint8_t {
	eLinear,
	eStep,
};

template <typename T>
struct Interpolator {
	using value_type = T;

	struct Keyframe {
		T value{};
		Duration timestamp{};
	};

	std::vector<Keyframe> keyframes{};
	Interpolation interpolation{};

	[[nodiscard]] auto duration() const -> Duration { return keyframes.empty() ? Duration{} : keyframes.back().timestamp; }

	[[nodiscard]] auto index_for(Duration time) const -> std::optional<std::size_t> {
		if (keyframes.empty()) { return {}; }

		auto const it = std::lower_bound(keyframes.begin(), keyframes.end(), time, [](Keyframe const& k, Duration time) { return k.timestamp < time; });
		if (it == keyframes.end()) { return {}; }
		return static_cast<std::size_t>(it - keyframes.begin());
	}

	auto operator()(Duration elapsed) const -> std::optional<T> {
		if (keyframes.empty()) { return {}; }

		auto const i_next = index_for(elapsed);
		if (!i_next) { return keyframes.back().value; }

		assert(*i_next < keyframes.size());
		auto const& next = keyframes[*i_next];
		assert(elapsed <= next.timestamp);
		if (*i_next == 0) { return next.value; }

		auto const& prev = keyframes[*i_next - 1];
		assert(prev.timestamp < elapsed);
		if (interpolation == Interpolation::eStep) { return prev.value; }

		auto const t = (elapsed - prev.timestamp) / (next.timestamp - prev.timestamp);
		using graphics::lerp;
		using std::lerp;
		return lerp(prev.value, next.value, t);
	}
};
} // namespace spaced::graphics
