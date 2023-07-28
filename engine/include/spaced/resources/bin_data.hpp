#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace spaced {
template <typename Type>
concept BinaryT = std::is_standard_layout_v<Type>;

struct BinSign {
	std::uint32_t value{};

	auto operator==(BinSign const&) const -> bool = default;
};

struct BinReader {
	std::span<std::uint8_t const> bytes{};

	template <BinaryT Type>
	auto read(std::span<Type> out) -> bool {
		if (out.empty()) { return true; }
		if (bytes.size() < out.size_bytes()) { return false; }
		std::memcpy(out.data(), bytes.data(), out.size_bytes());
		bytes = bytes.subspan(out.size_bytes());
		return true;
	}
};

struct BinWriter {
	// NOLINTNEXTLINE
	std::vector<std::uint8_t>& out_bytes;

	template <BinaryT Type>
	auto write(std::span<Type> data) -> BinWriter& {
		if (data.empty()) { return *this; }
		auto const start = out_bytes.size();
		out_bytes.resize(out_bytes.size() + data.size_bytes());
		auto const span = std::span{out_bytes}.subspan(start);
		assert(span.size() >= data.size_bytes());
		std::memcpy(span.data(), data.data(), data.size_bytes());
		return *this;
	}
};
} // namespace spaced
