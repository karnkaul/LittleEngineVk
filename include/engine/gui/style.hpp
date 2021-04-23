#pragma once
#include <optional>
#include <core/std_types.hpp>

namespace le::gui {
enum class Status { eIdle, eHover, ePress, eHold, eRelease, eCOUNT_ };

template <typename T>
struct Style {
	using type = T;

	EnumArray<Status, std::optional<T>> overrides;
	type base{};

	constexpr type const& operator[](Status s) const noexcept;
	constexpr type& at(Status s) noexcept;
	constexpr void set(Status s, T t) noexcept;
	constexpr void reset(Status s) noexcept;
};

// impl

template <typename T>
constexpr T const& Style<T>::operator[](Status s) const noexcept {
	return overrides[(std::size_t)s].has_value() ? *overrides[(std::size_t)s] : base;
}

template <typename T>
constexpr T& Style<T>::at(Status s) noexcept {
	auto& o = overrides[(std::size_t)s];
	if (!o) {
		o = base;
	}
	return *o;
}

template <typename T>
constexpr void Style<T>::set(Status s, T t) noexcept {
	overrides[(std::size_t)s] = std::move(t);
}

template <typename T>
constexpr void Style<T>::reset(Status s) noexcept {
	overrides[(std::size_t)s].reset();
}
} // namespace le::gui
