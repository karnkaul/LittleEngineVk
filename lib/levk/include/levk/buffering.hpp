#pragma once
#include <array>

namespace levk {
/// \brief Buffering for in-flight frames and volatile resources.
inline constexpr auto buffering_v = std::size_t{3};

/// \brief Buffered storage.
template <typename Type>
using Buffered = std::array<Type, buffering_v>;

/// \brief Wrapper for buffered index.
/// Managed by RenderDevice.
struct FrameIndex {
	std::size_t value{};

	constexpr void increment() { value = (value + 1) % buffering_v; }
	constexpr operator std::size_t() const { return value; }
};
} // namespace levk
