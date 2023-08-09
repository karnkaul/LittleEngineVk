#pragma once
#include <algorithm>
#include <ratio>

namespace le {
template <typename ResolutionT>
class BasicGain {
  public:
	using resolution_t = ResolutionT;
	static constexpr auto resolution_v = static_cast<float>(ResolutionT::num) / static_cast<float>(ResolutionT::den);

	static constexpr auto min_value_v{0.0f * resolution_v};
	static constexpr auto max_value_v{1.0f * resolution_v};

	BasicGain() = default;

	explicit constexpr BasicGain(float const value) : gain(std::clamp(value / resolution_v, 0.0f, 1.0f)) {}

	template <typename T>
	constexpr BasicGain(BasicGain<T> const& other) : gain(other.normalized()) {}

	[[nodiscard]] constexpr auto value() const -> float { return gain * resolution_v; }
	[[nodiscard]] constexpr auto normalized() const -> float { return gain; }

	auto operator<=>(BasicGain const&) const = default;

  private:
	float gain{};
};

using Gain = BasicGain<std::ratio<1>>;
using Volume = BasicGain<std::hecto>;
} // namespace le
