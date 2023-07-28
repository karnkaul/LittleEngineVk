#pragma once
#include <array>
#include <cstdint>

namespace spaced::graphics {
constexpr std::size_t buffering_v{2};

template <typename Type>
using Buffered = std::array<Type, buffering_v>;

struct FrameIndex {
	std::size_t value{};

	constexpr auto increment() -> void { value = (value + 1) % buffering_v; }

	constexpr operator std::size_t() const { return value; }
};

template <typename BufferedT, typename MakeT>
auto fill_buffered(BufferedT& out, MakeT make) -> void {
	for (auto& type : out) { type = make(); }
}

template <typename Type, typename MakeT>
auto make_buffered(MakeT make) -> Buffered<Type> {
	auto ret = Buffered<Type>{};
	fill_buffered(ret, std::move(make));
	return ret;
}
} // namespace spaced::graphics
