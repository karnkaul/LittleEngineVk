#pragma once

namespace levk {
/// \brief Wrapper for constructing overloaded visitors for std::visit.
template <typename... T>
struct Visitor : T... {
	using T::operator()...;
};
} // namespace levk
