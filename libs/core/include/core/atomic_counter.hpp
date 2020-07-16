#pragma once
#include <atomic>
#include <core/std_types.hpp>

namespace le
{
///
/// \brief Simple counter that increments/decrements atomically
///
template <typename T = s64>
struct Counter final
{
	struct Semaphore;

	std::atomic<T> counter;

	void increment();
	void decrement();

	///
	/// \brief Check if counter value is 0
	/// \param bAllowNegative Return true even if counter is negative
	///
	bool isZero(bool bAllowNegative) const;
};

template <typename T>
void Counter<T>::increment()
{
	++counter;
}

template <typename T>
void Counter<T>::decrement()
{
	--counter;
}

template <typename T>
bool Counter<T>::isZero(bool bAllowNegative) const
{
	return bAllowNegative ? counter.load() <= 0 : counter.load() == 0;
}

///
/// \brief Simple counting semaphore that uses Counter<T>
///
template <typename T>
struct Counter<T>::Semaphore final
{
	Counter<T>* pCounter = nullptr;

	Semaphore() = default;

	///
	/// \brief Stores and increments passed counter
	///
	Semaphore(Counter<T>& counter) : pCounter(&counter)
	{
		counter.increment();
	}
	Semaphore(Semaphore&&) = default;
	Semaphore& operator=(Semaphore&&) = default;
	Semaphore(Semaphore const&) = delete;
	Semaphore& operator=(Semaphore const&) = delete;
	///
	/// \brief Decrements stored counter (if any)
	///
	~Semaphore()
	{
		if (pCounter)
		{
			pCounter->decrement();
		}
	}

	///
	/// \brief Resets stored counter
	///
	void reset()
	{
		pCounter = nullptr;
	}
};
} // namespace le
