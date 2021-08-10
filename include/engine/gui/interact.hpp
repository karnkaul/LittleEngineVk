#pragma once
#include <optional>
#include <core/std_types.hpp>

namespace le::gui {
enum class InteractStatus { eIdle, eInactive, eHover, ePress, eHold, eRelease, eCOUNT_ };

template <typename T>
struct InteractStyle {
	using type = T;

	EnumArray<InteractStatus, std::optional<T>> overrides;
	type base{};

	constexpr type const& operator[](InteractStatus s) const noexcept;
	constexpr type& at(InteractStatus s) noexcept;
	constexpr void set(InteractStatus s, T t) noexcept;
	constexpr void reset(InteractStatus s) noexcept;
	constexpr void reset() noexcept;
};

// impl

template <typename T>
constexpr T const& InteractStyle<T>::operator[](InteractStatus s) const noexcept {
	return overrides[s].has_value() ? *overrides[s] : base;
}

template <typename T>
constexpr T& InteractStyle<T>::at(InteractStatus s) noexcept {
	auto& o = overrides[s];
	if (!o) { o = base; }
	return *o;
}

template <typename T>
constexpr void InteractStyle<T>::set(InteractStatus s, T t) noexcept {
	overrides[s] = std::move(t);
}

template <typename T>
constexpr void InteractStyle<T>::reset(InteractStatus s) noexcept {
	overrides[s].reset();
}

template <typename T>
constexpr void InteractStyle<T>::reset() noexcept {
	for (auto& o : overrides.arr) { o.reset(); }
}
} // namespace le::gui
