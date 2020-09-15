#pragma once
#include <atomic>
#include <core/std_types.hpp>

namespace le
{
///
/// \brief Simple counter that increments/decrements atomically
///
template <typename T = s64>
struct TCounter final
{
	struct Semaphore;

	void increment() noexcept;
	void decrement() noexcept;

	///
	/// \brief Check if counter value is 0
	/// \param bAllowNegative Return true even if counter is negative
	///
	bool isZero(bool bAllowNegative) const noexcept;

	std::atomic<T> counter;
};

///
/// \brief Simple counting semaphore that uses TCounter<T>
///
template <typename T>
struct TCounter<T>::Semaphore final
{
public:
	constexpr Semaphore() noexcept = default;
	///
	/// \brief Stores and increments passed counter
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
void TCounter<T>::increment() noexcept
{
	++counter;
}

template <typename T>
void TCounter<T>::decrement() noexcept
{
	--counter;
}

template <typename T>
bool TCounter<T>::isZero(bool bAllowNegative) const noexcept
{
	return bAllowNegative ? counter.load() <= 0 : counter.load() == 0;
}

template <typename T>
TCounter<T>::Semaphore::Semaphore(TCounter<T>& counter) noexcept : pTCounter(&counter)
{
	counter.increment();
}

template <typename T>
TCounter<T>::Semaphore::Semaphore(Semaphore&& rhs) noexcept : pTCounter(rhs.pTCounter)
{
	rhs.pTCounter = nullptr;
}

template <typename T>
typename TCounter<T>::Semaphore& TCounter<T>::Semaphore::operator=(Semaphore&& rhs) noexcept
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
TCounter<T>::Semaphore::~Semaphore()
{
	reset();
}

template <typename T>
void TCounter<T>::Semaphore::reset() noexcept
{
	if (pTCounter)
	{
		pTCounter->decrement();
	}
	pTCounter = nullptr;
}
} // namespace le
