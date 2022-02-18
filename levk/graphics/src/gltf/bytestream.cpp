#include <levk/graphics/gltf/bytestream.hpp>

namespace le::gltf {
std::vector<std::byte> base64_decode(std::string_view const base64) {
	static constexpr unsigned char table[] = {
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64, 64, 0,	1,	2,	3,	4,	5,	6,	7,	8,
		9,	10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64, 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
		64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64};

	std::size_t const in_len = base64.size();
	assert(in_len % 4 == 0);
	if (in_len % 4 != 0) { return {}; }

	std::size_t out_len = in_len / 4 * 3;
	if (base64[in_len - 1] == '=') out_len--;
	if (base64[in_len - 2] == '=') out_len--;

	std::vector<std::byte> ret;
	ret.resize(out_len);

	for (std::size_t i = 0, j = 0; i < in_len;) {
		std::uint32_t const a = base64[i] == '=' ? 0 & i++ : table[static_cast<std::size_t>(base64[i++])];
		std::uint32_t const b = base64[i] == '=' ? 0 & i++ : table[static_cast<std::size_t>(base64[i++])];
		std::uint32_t const c = base64[i] == '=' ? 0 & i++ : table[static_cast<std::size_t>(base64[i++])];
		std::uint32_t const d = base64[i] == '=' ? 0 & i++ : table[static_cast<std::size_t>(base64[i++])];

		std::uint32_t const triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);

		if (j < out_len) ret[j++] = static_cast<std::byte>((triple >> 2 * 8) & 0xFF);
		if (j < out_len) ret[j++] = static_cast<std::byte>((triple >> 1 * 8) & 0xFF);
		if (j < out_len) ret[j++] = static_cast<std::byte>((triple >> 0 * 8) & 0xFF);
	}

	return ret;
}
} // namespace le::gltf
