#pragma once
#include <algorithm>
#include <numbers>
#include <type_traits>
#include <core/maths.hpp>
#include <core/time.hpp>

namespace le::utils {
struct TweenLerp {
	template <typename T>
	constexpr T operator()(T const& lo, T const& hi, f32 t) noexcept(std::is_nothrow_move_constructible_v<T>) {
		return maths::lerp(lo, hi, t);
	}
};

struct TweenEase {
	template <typename T>
	constexpr T operator()(T const& lo, T const& hi, f32 t) noexcept(std::is_nothrow_move_constructible_v<T>) {
		return maths::lerp(lo, hi, 1.0f - (0.5f * (std::cos(t * std::numbers::pi_v<f32>) + 1.0f)));
	}
};

enum class TweenCycle { eOnce, eSwing, eLoop };

template <typename T, typename Tl = TweenLerp>
class Tweener {
  public:
	using value_type = T;
	using tween_t = Tl;

	constexpr Tweener(T lo, T hi, Time_s duration, TweenCycle cycle = {}, T current = T{}) noexcept(std::is_nothrow_move_constructible_v<T>)
		: m_lo(std::move(lo)), m_hi(std::move(hi)), m_curr(std::move(current)), m_dur(duration), m_cycle(cycle) {}

	constexpr T const& tick(Time_s dt) noexcept(std::is_nothrow_move_constructible_v<T>);
	constexpr T const& current() const noexcept { return m_curr; }
	constexpr bool complete() const noexcept { return m_cycle == TweenCycle::eOnce && m_elapsed >= m_dur; }
	constexpr TweenCycle cycle() const noexcept { return m_cycle; }

  private:
	T m_lo;
	T m_hi;
	T m_curr;
	Time_s m_dur;
	Time_s m_elapsed{};
	TweenCycle m_cycle;
	bool m_reverse = {};
};

// impl

template <typename T, typename Tl>
constexpr T const& Tweener<T, Tl>::tick(Time_s dt) noexcept(std::is_nothrow_move_constructible_v<T>) {
	switch (m_cycle) {
	case TweenCycle::eOnce:
		m_elapsed += dt;
		if (m_elapsed >= m_dur) { return m_hi; }
		break;
	case TweenCycle::eLoop:
		m_elapsed += dt;
		if (m_elapsed >= m_dur) { m_elapsed = {}; }
		break;
	case TweenCycle::eSwing: {
		if ((m_reverse && m_elapsed <= Time_s{}) || (!m_reverse && m_elapsed >= m_dur)) {
			m_elapsed = m_reverse ? Time_s{} : m_dur;
			m_reverse = !m_reverse;
		}
		m_elapsed += (m_reverse ? -dt : dt);
		break;
	}
	}
	return m_curr = std::clamp(Tl{}(m_lo, m_hi, std::clamp(m_elapsed / m_dur, 0.0f, 1.0f)), m_lo, m_hi);
}
} // namespace le::utils
