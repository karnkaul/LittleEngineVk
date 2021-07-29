#pragma once
#include <array>
#include <vector>
#include <core/ensure.hpp>
#include <ktl/fixed_vector.hpp>

namespace le {
///
/// \brief View-only class for an object / a contiguous range of objects
///
template <typename T>
class Span {
	static_assert(!std::is_reference_v<T>, "T cannot be a reference type");

  public:
	using value_type = T;
	using iterator = T*;
	using const_iterator = T const*;

	constexpr Span() noexcept;
	constexpr explicit Span(T* data, std::size_t size) noexcept;
	constexpr Span(T& data) noexcept;
	template <std::size_t N>
	constexpr Span(std::array<T, N>& arr) noexcept;
	template <std::size_t N>
	constexpr Span(ktl::fixed_vector<T, N>& vec) noexcept;
	template <std::size_t N>
	constexpr Span(T (&arr)[N]) noexcept;
	constexpr Span(std::vector<T>& vec) noexcept;

	constexpr std::size_t size() const noexcept;
	constexpr std::size_t size_bytes() const noexcept;
	constexpr T const* data() const noexcept;
	constexpr T* data() noexcept;
	constexpr bool empty() const noexcept;

	constexpr const_iterator begin() const noexcept;
	constexpr const_iterator end() const noexcept;
	constexpr iterator begin() noexcept;
	constexpr iterator end() noexcept;

	constexpr T const& front() const noexcept;
	constexpr T const& back() const noexcept;
	constexpr T& front() noexcept;
	constexpr T& back() noexcept;
	constexpr Span<T> subspan(std::size_t offset, std::size_t count) const noexcept;

	constexpr T const& operator[](std::size_t idx) const noexcept;
	constexpr T& operator[](std::size_t idx) noexcept;

  private:
	T* m_data;
	std::size_t m_size;
};

///
/// \brief Explicit specialization for Span<T const>
///
template <typename T>
class Span<T const> {
	static_assert(!std::is_reference_v<T>, "T cannot be a reference type");

  public:
	using value_type = T;
	using const_iterator = T const*;

	constexpr Span() noexcept;
	constexpr Span(Span<T> rhs) noexcept;
	constexpr explicit Span(T const* data, std::size_t size) noexcept;
	constexpr Span(T const& data) noexcept;
	template <std::size_t N>
	constexpr Span(std::array<T, N> const& arr) noexcept;
	template <std::size_t N>
	constexpr Span(ktl::fixed_vector<T, N> const& vec) noexcept;
	template <std::size_t N>
	constexpr Span(T const (&arr)[N]) noexcept;
	constexpr Span(std::vector<T> const& vec) noexcept;

	constexpr std::size_t size() const noexcept;
	constexpr std::size_t size_bytes() const noexcept;
	constexpr T const* data() const noexcept;
	constexpr bool empty() const noexcept;

	constexpr const_iterator begin() const noexcept;
	constexpr const_iterator end() const noexcept;

	constexpr T const& front() const noexcept;
	constexpr T const& back() const noexcept;
	constexpr Span<T const> subspan(std::size_t offset, std::size_t count) const noexcept;

	constexpr T const& operator[](std::size_t idx) const noexcept;

  private:
	T const* m_data;
	std::size_t m_size;
};

// impl

template <typename T>
constexpr Span<T>::Span() noexcept : m_data(nullptr), m_size(0) {}
template <typename T>
constexpr Span<T>::Span(T* data, std::size_t size) noexcept : m_data(data), m_size(size) {}
template <typename T>
constexpr Span<T>::Span(T& data) noexcept : m_data(&data), m_size(1) {}
template <typename T>
template <std::size_t N>
constexpr Span<T>::Span(std::array<T, N>& arr) noexcept : m_data(N == 0 ? nullptr : &arr.front()), m_size(N) {}
template <typename T>
template <std::size_t N>
constexpr Span<T>::Span(ktl::fixed_vector<T, N>& vec) noexcept : m_data(vec.empty() ? nullptr : &vec.front()), m_size(vec.size()) {}
template <typename T>
template <std::size_t N>
constexpr Span<T>::Span(T (&arr)[N]) noexcept : m_data(N == 0 ? nullptr : &arr[0]), m_size(N) {}
template <typename T>
constexpr Span<T>::Span(std::vector<T>& vec) noexcept : m_data(vec.empty() ? nullptr : &vec.front()), m_size(vec.size()) {}
template <typename T>
constexpr std::size_t Span<T>::size() const noexcept {
	return m_size;
}
template <typename T>
constexpr std::size_t Span<T>::size_bytes() const noexcept {
	return m_size * sizeof(T);
}
template <typename T>
constexpr bool Span<T>::empty() const noexcept {
	return m_size == 0;
}
template <typename T>
constexpr T const* Span<T>::data() const noexcept {
	return m_data;
}
template <typename T>
constexpr T* Span<T>::data() noexcept {
	return m_data;
}
template <typename T>
constexpr typename Span<T>::const_iterator Span<T>::begin() const noexcept {
	return m_data;
}
template <typename T>
constexpr typename Span<T>::const_iterator Span<T>::end() const noexcept {
	return m_data + m_size;
}
template <typename T>
constexpr typename Span<T>::iterator Span<T>::begin() noexcept {
	return m_data;
}
template <typename T>
constexpr typename Span<T>::iterator Span<T>::end() noexcept {
	return m_data + m_size;
}
template <typename T>
constexpr T const& Span<T>::front() const noexcept {
	ensure(!empty(), "OOB access!");
	return *m_data;
}
template <typename T>
constexpr T const& Span<T>::back() const noexcept {
	ensure(!empty(), "OOB access!");
	return *(m_data + m_size - 1);
}
template <typename T>
constexpr T& Span<T>::front() noexcept {
	ensure(!empty(), "OOB access!");
	return *m_data;
}
template <typename T>
constexpr T& Span<T>::back() noexcept {
	ensure(!empty(), "OOB access!");
	return *(m_data + m_size - 1);
}
template <typename T>
constexpr T const& Span<T>::operator[](std::size_t idx) const noexcept {
	ensure(idx < m_size, "OOB access!");
	return *(m_data + idx);
}
template <typename T>
constexpr T& Span<T>::operator[](std::size_t idx) noexcept {
	ensure(idx < m_size, "OOB access!");
	return *(m_data + idx);
}
template <typename T>
constexpr Span<T> Span<T>::subspan(std::size_t offset, std::size_t count) const noexcept {
	ensure(offset + count < m_size, "OOB access!");
	return Span<T>(m_data + offset, count);
}
template <typename T>
constexpr Span<T const>::Span() noexcept : m_data(nullptr), m_size(0) {}
template <typename T>
constexpr Span<T const>::Span(Span<T> rhs) noexcept : m_data(rhs.data()), m_size(rhs.size()) {}
template <typename T>
constexpr Span<T const>::Span(T const* data, std::size_t size) noexcept : m_data(data), m_size(size) {}
template <typename T>
constexpr Span<T const>::Span(T const& data) noexcept : m_data(&data), m_size(1) {}
template <typename T>
template <std::size_t N>
constexpr Span<T const>::Span(std::array<T, N> const& arr) noexcept : m_data(N == 0 ? nullptr : &arr.front()), m_size(N) {}
template <typename T>
template <std::size_t N>
constexpr Span<T const>::Span(ktl::fixed_vector<T, N> const& vec) noexcept : m_data(vec.empty() ? nullptr : &vec.front()), m_size(vec.size()) {}
template <typename T>
template <std::size_t N>
constexpr Span<T const>::Span(T const (&arr)[N]) noexcept : m_data(N == 0 ? nullptr : &arr[0]), m_size(N) {}
template <typename T>
constexpr Span<T const>::Span(std::vector<T> const& vec) noexcept : m_data(vec.empty() ? nullptr : &vec.front()), m_size(vec.size()) {}
template <typename T>
constexpr std::size_t Span<T const>::size() const noexcept {
	return m_size;
}
template <typename T>
constexpr std::size_t Span<T const>::size_bytes() const noexcept {
	return m_size * sizeof(T);
}
template <typename T>
constexpr bool Span<T const>::empty() const noexcept {
	return m_size == 0;
}
template <typename T>
constexpr T const* Span<T const>::data() const noexcept {
	return m_data;
}
template <typename T>
constexpr typename Span<T const>::const_iterator Span<T const>::begin() const noexcept {
	return m_data;
}
template <typename T>
constexpr typename Span<T const>::const_iterator Span<T const>::end() const noexcept {
	return m_data + m_size;
}
template <typename T>
constexpr T const& Span<T const>::front() const noexcept {
	ensure(!empty(), "OOB access!");
	return *m_data;
}
template <typename T>
constexpr T const& Span<T const>::back() const noexcept {
	ensure(!empty(), "OOB access!");
	return *(m_data + m_size - 1);
}
template <typename T>
constexpr Span<T const> Span<T const>::subspan(std::size_t offset, std::size_t count) const noexcept {
	ensure(offset + count < m_size, "OOB access!");
	return Span<T const>(m_data + offset, count);
}
template <typename T>
constexpr T const& Span<T const>::operator[](std::size_t idx) const noexcept {
	ensure(idx < m_size, "OOB access!");
	return *(m_data + idx);
}
} // namespace le
