#include <le/resources/bin_data.hpp>

[[nodiscard]] auto le::resize_and_overwrite(std::vector<std::byte>& out_bytes, std::span<std::byte const> bytes) -> OffsetSpan {
	auto ret = OffsetSpan{.offset = out_bytes.size(), .count = bytes.size()};
	out_bytes.resize(out_bytes.size() + bytes.size_bytes());
	auto const span = std::span{out_bytes}.subspan(ret.offset);
	assert(span.size() >= bytes.size_bytes());
	std::memcpy(span.data(), bytes.data(), bytes.size_bytes());
	return ret;
}
