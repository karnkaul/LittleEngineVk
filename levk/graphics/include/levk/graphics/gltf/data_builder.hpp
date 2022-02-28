#pragma once
#include "bytestream.hpp"

namespace le::gltf {
template <typename T, std::size_t N>
struct tvec {
	T data[N];
};

template <typename T, std::size_t N>
struct tmat {
	tvec<T, N> data[N];
};

template <typename T>
using tvec2 = tvec<T, 2>;
template <typename T>
using tvec3 = tvec<T, 3>;
template <typename T>
using tvec4 = tvec<T, 4>;

using quat_t = tvec4<float>;

template <typename T>
using tmat2 = tmat<T, 2>;
template <typename T>
using tmat3 = tmat<T, 3>;
template <typename T>
using tmat4 = tmat<T, 4>;

using buffer_span = std::span<std::byte const>;

template <typename T>
struct data_builder;

template <typename Ret, typename In = Ret>
	requires(std::is_convertible_v<In, Ret>)
std::vector<Ret> build_data(buffer_span buffer, std::size_t stride = 0U, std::endian const e = std::endian::little) {
	auto in = data_builder<In>{}(buffer, stride, e);
	if constexpr (!std::is_same_v<In, Ret>) {
		std::vector<Ret> ret;
		ret.reserve(in.size());
		std::copy(in.begin(), in.end(), std::back_inserter(ret));
		return ret;
	} else {
		return in;
	}
}

// impl

template <typename T, std::size_t N>
constexpr buffer_span read_tvec(buffer_span in, tvec<T, N>& out, std::size_t stride = 0U, std::endian e = std::endian::little) {
	assert(in.size() >= sizeof(T) * N);
	assert(stride == 0U || stride >= sizeof(T) * N);
	for (std::size_t i = 0; i < N; ++i) { in = read(e, in, out.data[i]); }
	return in.subspan(stride);
}

template <typename T, std::size_t N>
constexpr buffer_span read_tmat(buffer_span in, tmat<T, N>& out, std::size_t stride = 0U, std::endian e = std::endian::little) {
	assert(in.size() >= sizeof(T) * N * N);
	assert(stride == 0U || stride >= sizeof(T) * N * N);
	for (std::size_t i = 0; i < N; ++i) { in = read_tvec(in, out.data[i], 0U, e); }
	return in.subspan(stride);
}

template <typename T>
struct data_builder {
	std::vector<T> operator()(buffer_span buffer, std::size_t stride, std::endian const e) const {
		assert(buffer.size() % sizeof(T) == 0);
		assert(stride == 0 || stride > sizeof(T));
		std::vector<T> ret;
		ret.reserve(buffer.size() / sizeof(T));
		for (auto head = std::span(buffer); !head.empty();) {
			T t;
			head = read(e, head, t);
			ret.push_back(std::move(t));
			head = head.subspan(stride);
		}
		return ret;
	}
};

template <typename T, std::size_t N>
struct data_builder<tvec<T, N>> {
	std::vector<tvec<T, N>> operator()(buffer_span buffer, std::size_t stride, std::endian const e) const {
		assert(buffer.size() % (sizeof(T) * N) == 0);
		std::vector<tvec<T, N>> ret;
		ret.reserve(buffer.size() / (sizeof(T) * N));
		for (auto head = std::span(buffer); !head.empty();) {
			tvec<T, N> vec;
			head = read_tvec(head, vec, stride, e);
			ret.push_back(vec);
			head = head.subspan(stride);
		}
		return ret;
	}
};

template <typename T, std::size_t N>
struct data_builder<tmat<T, N>> {
	std::vector<tmat<T, N>> operator()(buffer_span buffer, std::size_t stride, std::endian const e) const {
		assert(buffer.size() % (sizeof(T) * N * N) == 0);
		std::vector<tmat<T, N>> ret;
		ret.reserve(buffer.size() / (sizeof(T) * N * N));
		for (auto head = std::span(buffer); !head.empty();) {
			assert(head.size() >= sizeof(T) * N);
			tmat<T, N> mat;
			head = read_tmat(head, mat, stride, e);
			ret.push_back(mat);
		}
		return ret;
	}
};
} // namespace le::gltf
