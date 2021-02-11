#pragma once
#include <functional>
#include <core/ref.hpp>
#include <core/zero.hpp>

namespace std {
///
/// \brief std::hash<Ref<T>> partial specialization
///
template <typename T>
struct hash<le::Ref<T>> {
	size_t operator()(le::Ref<T> const& lhs) const {
		return std::hash<T const*>()(&lhs.get());
	}
};

///
/// \brief std::hash<TZero<T, Zero>> partial specialization
///
template <typename T, T Zero>
struct hash<le::TZero<T, Zero>> {
	size_t operator()(le::TZero<T, Zero> zero) const noexcept {
		return std::hash<T>()(zero.payload);
	}
};
} // namespace std
