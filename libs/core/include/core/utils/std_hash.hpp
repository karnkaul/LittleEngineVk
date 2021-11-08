#pragma once
#include <core/io/path.hpp>
#include <core/ref.hpp>
#include <functional>

namespace std {
///
/// \brief std::hash<Ref<T>> partial specialization
///
template <typename T>
struct hash<le::Ref<T>> {
	size_t operator()(le::Ref<T> const& lhs) const { return std::hash<T>()(lhs.get()); }
};

///
/// \brief std::hash<io::Path> partial specialization
///
template <>
struct hash<le::io::Path> {
	size_t operator()(le::io::Path const& path) const { return std::hash<std::string>()(path.generic_string()); }
};
} // namespace std
