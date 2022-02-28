#pragma once
#include <algorithm>
#include <bit>
#include <cassert>
#include <cstring>
#include <iterator>
#include <span>

namespace le::gltf {
template <std::size_t Count>
constexpr void copy_to(std::endian const e, std::byte const* in, std::byte* out) {
	if (e == std::endian::little) {
		std::copy(in, in + Count, out);
	} else {
		std::reverse_copy(in, in + Count, out);
	}
}

template <typename T>
constexpr std::span<std::byte const> read(std::endian const e, std::span<std::byte const> in, T& out) {
	assert(in.size() >= sizeof(T));
	std::byte buf[sizeof(T)];
	copy_to<sizeof(T)>(e, in.data(), buf);
	std::memcpy(std::addressof(out), buf, sizeof(T));
	return in.subspan(sizeof(T));
}

template <typename T>
constexpr std::span<std::byte> write(std::endian const e, std::span<std::byte> out, T const& t) {
	assert(out.size() >= sizeof(T));
	std::byte buf[sizeof(T)];
	std::memcpy(buf, std::addressof(t), sizeof(T));
	copy_to<sizeof(T)>(e, buf, out.data());
	return out.subspan(sizeof(T));
}

template <typename T, std::output_iterator<T> It>
constexpr void read_to(std::endian const e, std::span<std::byte const> in, It out) {
	assert(in.size() % sizeof(T) == 0);
	while (!in.empty()) {
		T t;
		in = read(e, in, t);
		*out++ = std::move(t);
	}
}

template <typename T, std::output_iterator<T> It>
constexpr void write_to(std::endian const e, std::span<T const> in, It out) {
	for (T const& t : in) {
		std::byte buf[sizeof(T)];
		write(e, buf, t);
		for (std::byte const byte : buf) { *out++ = byte; }
	}
}

template <typename T, typename Out, std::size_t... I>
constexpr std::span<T const> write_members(std::index_sequence<I...> seq, std::span<T const> data, Out& out) {
	assert(data.size() >= seq.size());
	out = {data[I]...};
	return data.subspan(seq.size());
}

template <typename T, std::size_t Count, typename Out>
constexpr std::span<T const> write_members(std::span<T const> data, Out& out) {
	return write_members<T, Out>(std::make_index_sequence<Count>(), data, out);
}

template <typename T, std::size_t Size, std::size_t Count = Size, typename Out>
constexpr std::span<T const> write_array_members(std::span<T const> data, Out& out) {
	for (std::size_t idx = 0; idx < Size; ++idx) { data = write_members<T, Count>(data, out[idx]); }
	return data;
}

template <typename T>
std::vector<T> from_bytes(std::endian const e, std::span<std::byte const> bytes) {
	std::vector<T> ret;
	ret.reserve(bytes.size() / sizeof(T));
	read_to(e, bytes, std::back_inserter(ret));
	return ret;
}

template <typename T>
std::vector<std::byte> to_bytes(std::endian const e, std::span<T const> ts) {
	std::vector<std::byte> ret;
	ret.reserve(ts.size() * sizeof(T));
	write_to(e, ts, std::back_inserter(ret));
	return ret;
}

std::vector<std::byte> base64_decode(std::string_view const base64);
} // namespace le::gltf
