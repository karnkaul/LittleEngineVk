#pragma once
#include <atomic>
#include <core/std_types.hpp>

namespace le
{
///
/// \brief Generic template type (undefined)
///
template <typename T = s64, bool Atomic = true>
struct TCounter;

template <typename T>
constexpr bool is_atomic_counter_v = T::is_atomic;

///
/// \brief Simple counter that operator++s/operator--s atomically
///
template <typename T>
struct TCounter<T, true> final
{
	static_assert(std::is_integral_v<T>, "T must be integral!");

	using type = T;
	static constexpr bool is_atomic = true;

	struct Semaphore;

	TCounter(T init = {}) noexcept;

	T operator++() noexcept;
	T operator--() noexcept;
	T operator++(int) noexcept;
	T operator--(int) noexcept;

	///
	/// \brief Check if counter value is 0
	/// \param bAllowNegative Return true even if counter is negative
	///
	bool isZero(bool bAllowNegative) const noexcept;

	operator T() const noexcept;

	std::atomic<T> counter;
};

///
/// \brief Simple counter that operator++s/operator--s non-atomically
///
template <typename T>
struct TCounter<T, false> final
{
	static_assert(std::is_integral_v<T>, "T must be integral!");

	using type = T;
	static constexpr bool is_atomic = false;

	constexpr TCounter(T init = {}) noexcept;

	constexpr T operator++() noexcept;
	constexpr T operator--() noexcept;
	constexpr T operator++(int) noexcept;
	constexpr T operator--(int) noexcept;

	///
	/// \brief Check if counter value is 0
	/// \param bAllowNegative Return true even if counter is negative
	///
	constexpr bool isZero(bool bAllowNegative) const noexcept;

	constexpr operator T() const noexcept;

	T counter;
};

///
/// \brief Simple counting semaphore that uses TCounter<T>
///
template <typename T>
struct TCounter<T, true>::Semaphore final
{
public:
	constexpr Semaphore() noexcept = default;
	///
	/// \brief Stores and operator++s passed counter
	///
	Semaphore(TCounter<T>& counter) noexcept;
	///
	/// \brief Moves rhs into self
	///
	Semaphore(Semaphore&& rhs) noexcept;
	///
	/// \brief Decrements counter if tied to one, then moves rhs into self
	///
	Semaphore& operator=(Semaphore&& rhs) noexcept;
	Semaphore(Semaphore const&) = delete;
	Semaphore& operator=(Semaphore const&) = delete;
	///
	/// \brief Decrements stored counter (if any)
	///
	~Semaphore();

	///
	/// \brief Resets stored counter
	///
	void reset() noexcept;

private:
	TCounter<T>* pTCounter = nullptr;
};

template <typename T>
TCounter<T, true>::TCounter(T init) noexcept
{
	counter.store(init);
}

template <typename T>
T TCounter<T, true>::operator++() noexcept
{
	return ++counter;
}

template <typename T>
T TCounter<T, true>::operator--() noexcept
{
	return --counter;
}

template <typename T>
T TCounter<T, true>::operator++(int) noexcept
{
	return counter++;
}

template <typename T>
T TCounter<T, true>::operator--(int) noexcept
{
	return counter--;
}

template <typename T>
bool TCounter<T, true>::isZero(bool bAllowNegative) const noexcept
{
	return bAllowNegative ? counter.load() <= 0 : counter.load() == 0;
}

template <typename T>
TCounter<T, true>::operator T() const noexcept
{
	return counter.load();
}

template <typename T>
constexpr TCounter<T, false>::TCounter(T init) noexcept : counter(init)
{
}

template <typename T>
constexpr T TCounter<T, false>::operator++() noexcept
{
	return ++counter;
}

template <typename T>
constexpr T TCounter<T, false>::operator--() noexcept
{
	return --counter;
}

template <typename T>
constexpr T TCounter<T, false>::operator++(int) noexcept
{
	return counter++;
}

template <typename T>
constexpr T TCounter<T, false>::operator--(int) noexcept
{
	return counter--;
}

template <typename T>
constexpr bool TCounter<T, false>::isZero(bool bAllowNegative) const noexcept
{
	return bAllowNegative ? counter <= 0 : counter == 0;
}

template <typename T>
constexpr TCounter<T, false>::operator T() const noexcept
{
	return counter;
}

template <typename T>
TCounter<T, true>::Semaphore::Semaphore(TCounter<T>& counter) noexcept : pTCounter(&counter)
{
	++counter;
}

template <typename T>
TCounter<T, true>::Semaphore::Semaphore(Semaphore&& rhs) noexcept : pTCounter(rhs.pTCounter)
{
	rhs.pTCounter = nullptr;
}

template <typename T>
typename TCounter<T, true>::Semaphore& TCounter<T, true>::Semaphore::operator=(Semaphore&& rhs) noexcept
{
	if (&rhs != this)
	{
		if (rhs.pTCounter != pTCounter)
		{
			reset();
		}
		pTCounter = rhs.pTCounter;
		rhs.pTCounter = nullptr;
	}
	return *this;
}

template <typename T>
TCounter<T, true>::Semaphore::~Semaphore()
{
	reset();
}

template <typename T>
void TCounter<T, true>::Semaphore::reset() noexcept
{
	if (pTCounter)
	{
		--(*pTCounter);
	}
	pTCounter = nullptr;
}
} // namespace le
