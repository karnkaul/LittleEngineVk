#pragma once
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace spaced::bin {
static constexpr std::uint64_t version_v{1};

template <typename Type>
concept TrivialT = std::is_standard_layout_v<Type>;

struct Header {
	std::uint64_t version;
	std::uint64_t stride;
	std::uint64_t count;

	[[nodiscard]] constexpr auto is_valid() const -> bool { return version == version_v && stride > 0; }

	template <TrivialT Type>
	[[nodiscard]] constexpr auto matches_type() const -> bool {
		return is_valid() && sizeof(Type) == stride;
	}
};

struct Unpack {
	std::span<std::uint8_t const> bytes{};

	template <TrivialT Type>
	auto to(std::vector<Type>& ret) -> bool {
		if (bytes.size() < sizeof(Header)) { return false; }
		auto header = Header{};
		std::memcpy(&header, bytes.data(), sizeof(header));
		bytes = bytes.subspan(sizeof(header));
		auto const size_bytes = header.stride * header.count;
		if (!header.matches_type<Type>() || bytes.size_bytes() < size_bytes) { return false; }
		if (size_bytes == 0) { return true; }
		auto const start = ret.size();
		ret.resize(ret.size() + header.count);
		auto const span = std::span<Type>{ret}.subspan(start);
		std::memcpy(span.data(), bytes.data(), size_bytes);
		bytes = bytes.subspan(size_bytes);
		return true;
	}
};

struct Pack {
	std::vector<std::uint8_t> bytes{};

	template <TrivialT Type>
	auto pack(std::span<Type const> data) -> Pack& {
		auto const start = bytes.size();
		bytes.resize(bytes.size() + sizeof(Header) + data.size_bytes());
		auto const span = std::span<std::uint8_t>{bytes}.subspan(start);
		auto const header = Header{
			.version = version_v,
			.stride = sizeof(Type),
			.count = data.size(),
		};
		std::memcpy(span.data(), &header, sizeof(header));
		if (data.empty()) { return *this; }
		std::memcpy(span.subspan(sizeof(header)).data(), data.data(), data.size_bytes());
		return *this;
	}
};
} // namespace spaced::bin
