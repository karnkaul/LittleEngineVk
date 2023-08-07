#pragma once
#include <le/core/offset_span.hpp>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace le {
template <typename Type>
concept BinaryT = std::is_standard_layout_v<Type>;

struct BinSign {
	std::uint32_t value{};

	auto operator==(BinSign const&) const -> bool = default;
};

struct BinReader {
	std::span<std::byte const> bytes{};

	template <BinaryT Type>
	auto read(std::span<Type> out) -> bool {
		if (out.empty()) { return true; }
		if (bytes.size() < out.size_bytes()) { return false; }
		std::memcpy(out.data(), bytes.data(), out.size_bytes());
		bytes = bytes.subspan(out.size_bytes());
		return true;
	}
};

auto resize_and_overwrite(std::vector<std::byte>& out_bytes, std::span<std::byte const> bytes) -> OffsetSpan;

struct BinWriter {
	// NOLINTNEXTLINE
	std::vector<std::byte>& out_bytes;

	template <BinaryT Type>
	auto write(std::span<Type> data) -> BinWriter& {
		if (data.empty()) { return *this; }
		// NOLINTNEXTLINE
		auto const span = std::span<std::byte const>{reinterpret_cast<std::byte const*>(data.data()), data.size_bytes()};
		resize_and_overwrite(out_bytes, span);
		return *this;
	}
};
} // namespace le
