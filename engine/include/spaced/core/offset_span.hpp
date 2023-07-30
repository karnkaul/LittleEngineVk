#pragma once
#include <span>
#include <vector>

namespace spaced {
struct OffsetSpan {
	std::size_t offset{};
	std::size_t count{};

	[[nodiscard]] constexpr auto is_empty() const { return count == 0; }

	template <typename ContainerT>
	constexpr auto make_span(ContainerT&& container) const {
		using Type = std::remove_reference_t<decltype(container[0])>;
		return std::span<Type>{container}.subspan(offset, count);
	}
};
} // namespace spaced
