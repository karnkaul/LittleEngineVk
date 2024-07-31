#pragma once
#include <levk/core/error.hpp>
#include <levk/core/inclusive_range.hpp>
#include <levk/core/not_null.hpp>
#include <levk/core/time.hpp>
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <vector>

namespace levk {
/// \brief Animation interpolation type.
enum class LerpType : std::int8_t { eLinear, eStep };

/// \brief Concept for interpolators.
template <typename Type, typename ValueType>
concept InterpolatorT = requires(Type const& l, LerpType i, InclusiveRange<ValueType> const& r, float t) {
	{ l(i, r, t) } -> std::convertible_to<ValueType>;
};

/// \brief Animation keyframe.
template <typename Type>
struct AnimationKeyframe {
	Seconds timestamp{};
	Type output{};
};

/// \brief Default interpolator: uses std::lerp.
template <typename Type>
struct DefaultInterpolator {
	[[nodiscard]] constexpr auto operator()(LerpType const lerp_type, InclusiveRange<Type> const& range, float const alpha) const -> Type {
		if (lerp_type == LerpType::eStep) { return range.lo; }
		return std::lerp(range.lo, range.hi, alpha);
	}
};

static_assert(InterpolatorT<DefaultInterpolator<float>, float>);

/// \brief Animation Sampler: enables sampling based on input time. Output is clamped.
template <typename Type, InterpolatorT<Type> LerpT = DefaultInterpolator<Type>>
class AnimationSampler {
  public:
	using Keyframe = AnimationKeyframe<Type>;
	using value_type = Type;
	using interpolator_t = LerpT;

	static constexpr auto lerp_type_v = LerpType::eLinear;

	explicit AnimationSampler(LerpT lerp = LerpT{}) : m_lerp(std::move(lerp)) {}

	/// \brief Reserve space for count keyframes.
	void reserve(std::size_t const count) { m_keyframes.reserve(count); }

	/// \brief Add a keyframe.
	void add_keyframe(Keyframe keyframe) {
		auto const it = std::ranges::lower_bound(m_keyframes, keyframe, CompProj{});
		m_keyframes.insert(it, std::move(keyframe));
	}

	/// \brief Add a keyframe.
	void add_keyframe(Seconds timestamp, Type t) { add_keyframe(Keyframe{.timestamp = timestamp, .output = std::move(t)}); }

	/// \brief Set keyframes.
	/// \param sorted_keyframes Pre-sorted list of keyframes.
	void set_keyframes(std::vector<Keyframe> sorted_keyframes) { m_keyframes = std::move(sorted_keyframes); }

	/// \brief Clear all stored keyframes.
	void clear_keyframes() { m_keyframes.clear(); }

	[[nodiscard]] auto get_keyframes() const -> std::span<Keyframe const> { return m_keyframes; }
	[[nodiscard]] auto is_empty() const -> bool { return m_keyframes.empty(); }

	/// \brief Obtain the timestamp of the first keyframe.
	[[nodiscard]] auto get_entry() const -> Seconds { return m_keyframes.empty() ? Seconds{} : m_keyframes.front().timestamp; }
	/// \brief Obtain the timestamp of the last keyframe.
	[[nodiscard]] auto get_exit() const -> Seconds { return m_keyframes.empty() ? Seconds{} : m_keyframes.back().timestamp; }

	/// \brief Sample a value based on the given timestamp.
	/// \param elapsed Time elapsed since the start of the animation.
	/// \param fallback Falback value.
	/// \returns Interpolated sample, fallback if no keyframes are present.
	[[nodiscard]] auto sample(Seconds const elapsed, Type const& fallback = Type{}) const -> Type {
		if (is_empty()) { return fallback; }

		auto const it = std::ranges::lower_bound(m_keyframes, elapsed, {}, CompProj{});
		if (it == m_keyframes.end()) { return m_keyframes.back().output; }	  // last < elapsed
		if (it == m_keyframes.begin()) { return m_keyframes.front().output; } // first >= elapsed

		auto const& a = *(it - 1);
		auto const& b = *it;
		auto const delta = elapsed - a.timestamp;
		auto const total = b.timestamp - a.timestamp;
		auto const range = InclusiveRange<Type>{.lo = a.output, .hi = b.output};

		return m_lerp(lerp_type, range, delta / total);
	}

	/// \brief Interpolation type.
	LerpType lerp_type{lerp_type_v};

  private:
	struct CompProj {
		constexpr auto operator()(Keyframe const& a, Keyframe const& b) const { return a.timestamp < b.timestamp; }
		constexpr auto operator()(Keyframe const& keyframe) const -> Seconds { return keyframe.timestamp; }
	};

	LerpT m_lerp;
	std::vector<Keyframe> m_keyframes{};
};
} // namespace levk
