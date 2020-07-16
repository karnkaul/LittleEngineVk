#pragma once
#include <array>
#include <cstddef>
#include <cstring>
#include <typeinfo>
#include <core/assert.hpp>

namespace le
{
template <std::size_t N = sizeof(s64)>
class StaticAny final
{
private:
	std::array<std::byte, N> m_bytes;
	void (*m_destroy)(void const*);
	std::size_t m_hash = 0;

public:
	StaticAny() = default;

	template <typename T>
	StaticAny(T const& t)
	{
		static_assert(sizeof(T) < N, "Buffer overflow!");
		static_assert(std::is_constructible_v<T, T>, "T is not copy constructible");
		construct<T>(t);
	}

	template <typename T>
	StaticAny& operator=(T const& t)
	{
		static_assert(sizeof(T) < N, "Buffer overflow!");
		static_assert(std::is_constructible_v<T, T>, "T is not copy constructible");
		clear();
		construct<T>(t);
		return *this;
	}

	StaticAny(StaticAny&& rhs) = default;
	StaticAny& operator=(StaticAny&&) = default;
	StaticAny(StaticAny const& rhs) = default;
	StaticAny& operator=(StaticAny const&) = default;

	template <typename T>
	T get() const
	{
		if (typeid(T).hash_code() == m_hash)
		{
			return *reinterpret_cast<T*>(const_cast<std::byte*>(m_bytes.data()));
		}
		return {};
	}

	template <typename T>
	T* getPtr() const
	{
		if (typeid(T).hash_code() == m_hash)
		{
			return reinterpret_cast<T*>(const_cast<std::byte*>(m_bytes.data()));
		}
		return nullptr;
	}

	void clear()
	{
		if (m_destroy)
		{
			m_destroy(m_bytes.data());
		}
		m_hash = 0;
	}

private:
	template <typename T>
	void construct(T const& t)
	{
		T* pT = new (m_bytes.data()) T(t);
		m_destroy = [](void const* p) {
			auto pT = static_cast<T const*>(p);
			pT->~T();
		};
		m_hash = typeid(T).hash_code();
	}
};
} // namespace le
