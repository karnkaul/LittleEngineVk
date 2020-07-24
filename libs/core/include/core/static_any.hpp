#pragma once
#include <array>
#include <cstddef>
#include <optional>
#include <type_traits>

namespace le
{
template <std::size_t N = sizeof(void*)>
class StaticAny final
{
private:
	template <typename T>
	using is_different_t = std::enable_if_t<!std::is_same_v<T, StaticAny<N>>>;

	struct Erased
	{
	};

private:
	alignas(std::max_align_t) std::array<std::byte, N> m_bytes;
	Erased const* m_pErased = nullptr;

public:
	constexpr StaticAny() noexcept = default;

	template <typename T, typename = is_different_t<T>>
	StaticAny(T const& t)
	{
		construct<T>(t);
	}

	template <typename T, typename = is_different_t<T>>
	StaticAny& operator=(T const& t)
	{
		construct<T>(t);
		return *this;
	}

	template <typename T>
	bool contains() const noexcept
	{
		return erased<T>() == m_pErased;
	}

	template <typename T, typename = is_different_t<T>>
	std::optional<T> get() const
	{
		if (contains<T>())
		{
			return *reinterpret_cast<T const*>(m_bytes.data());
		}
		return {};
	}

	template <typename T, typename = is_different_t<T>>
	T const* getPtr() const noexcept
	{
		if (contains<T>() || contains<T const>())
		{
			return reinterpret_cast<T const*>(m_bytes.data());
		}
		return nullptr;
	}

	template <typename T, typename = is_different_t<T>>
	T* getPtr() noexcept
	{
		if (contains<T>())
		{
			return reinterpret_cast<T*>(m_bytes.data());
		}
		return nullptr;
	}

	void clear() noexcept
	{
		m_pErased = nullptr;
	}

private:
	template <typename T>
	void construct(T const& t)
	{
		if constexpr (std::is_same_v<T, std::nullptr_t>)
		{
			m_pErased = nullptr;
		}
		else
		{
			static_assert(sizeof(T) <= N, "Buffer overflow!");
			static_assert(std::is_trivially_copy_constructible_v<T> && std::is_trivially_destructible_v<T>, "T must be trivially copiable and destructible!");
			auto pErased = erased<T>();
			if (m_pErased == pErased)
			{
				T* pT = reinterpret_cast<T*>(m_bytes.data());
				*pT = t;
			}
			else
			{
				T* pT = new (m_bytes.data()) T{t};
				m_pErased = pErased;
			}
		}
	}

	template <typename T>
	static Erased const* erased()
	{
		static constexpr Erased s_erased;
		return &s_erased;
	}
};
} // namespace le
