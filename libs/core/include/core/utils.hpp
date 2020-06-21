#pragma once
#include <cstring>
#include <future>
#include <initializer_list>
#include <mutex>
#include <string>
#include <vector>
#include <typeinfo>
#include <type_traits>
#include <core/assert.hpp>
#include <core/std_types.hpp>

namespace le
{
enum class FutureState : s8
{
	eInvalid,
	eDeferred,
	eReady,
	eTimeout,
	eCOUNT_
};

///
/// \brief View-only class for an object / an array / `std::vector`
///
template <typename T>
struct Span
{
	using value_type = T;
	using const_iterator = T const*;

	T const* pData;
	size_t extent;

	constexpr Span() noexcept : pData(nullptr), extent(0) {}
	constexpr explicit Span(T const* pData, size_t extent) noexcept : pData(pData), extent(extent) {}
	constexpr Span(T const& data) noexcept : pData(&data), extent(1) {}

	template <typename Container>
	constexpr Span(Container const& container) noexcept : pData(container.empty() ? nullptr : &container.front()), extent(container.size())
	{
	}

	constexpr explicit Span(std::initializer_list<T> const& list) noexcept : pData(list.begin()), extent(list.size()) {}

	Span<T>(Span<T>&&) noexcept = default;
	Span<T>& operator=(Span<T>&&) noexcept = default;
	Span<T>(Span<T> const&) noexcept = default;
	Span<T>& operator=(Span<T> const&) noexcept = default;

	const_iterator begin() const
	{
		return pData;
	}

	const_iterator end() const
	{
		return pData + extent;
	}
};

///
/// \brief Class emulating a counting semaphore
///
template <typename T, T Zero = T{}>
class TSemaphore
{
public:
	///
	/// \brief Handle to owning Semaphore
	///
	class Handle
	{
	protected:
		T* m_pCounter;
		std::mutex* m_pMutex;
		std::condition_variable* m_pCV;

	public:
		Handle(T& counter, std::condition_variable& condVar, std::mutex& mutex);
		~Handle();
	};

public:
	///
	/// \brief Mutex for m_cv and m_counter (and user)
	///
	mutable std::mutex m_mutex;

protected:
	///
	/// \brief Used to wait until semaphore is idle
	///
	mutable std::condition_variable m_cv;
	///
	/// \brief Counter
	///
	mutable T m_counter = Zero;

public:
	///
	/// \brief Obtain new handle (and increment semaphore)
	/// \returns Handle to this Semaphore
	///
	Handle handle() const;
	///
	/// \brief Check if semaphore is idle
	/// \returns `true` if idle
	///
	bool isIdle() const;
	///
	/// \brief Wait until semaphore is idle
	///
	void waitIdle();
};

namespace utils
{
template <typename T>
FutureState futureState(std::future<T> const& future)
{
	if (future.valid())
	{
		using namespace std::chrono_literals;
		auto const status = future.wait_for(0ms);
		switch (status)
		{
		default:
		case std::future_status::deferred:
			return FutureState::eDeferred;
		case std::future_status::ready:
			return FutureState::eReady;
		case std::future_status::timeout:
			return FutureState::eTimeout;
		}
	}
	return FutureState::eInvalid;
}

template <typename T>
bool isReady(std::future<T> const& future)
{
	using namespace std::chrono_literals;
	return future.valid() && future.wait_for(0ms) == std::future_status::ready;
}

///
/// \brief Convert `byteCount` bytes into human-friendly format
/// \returns pair of size in `f32` and the corresponding unit
///
std::pair<f32, std::string_view> friendlySize(u64 byteCount);

///
/// \brief Demangle a compiler symbol name
///
std::string demangle(std::string_view name);

///
/// \brief Obtain demangled type name of an object
///
template <typename T>
std::string tName(T const& t)
{
	return demangle(typeid(t).name());
}

///
/// \brief Obtain demangled type name of a type
///
template <typename T>
std::string tName()
{
	return demangle(typeid(T).name());
}

namespace strings
{
// ASCII only
void toLower(std::string& outString);
void toUpper(std::string& outString);

bool toBool(std::string_view input, bool defaultValue = false);
s32 toS32(std::string_view input, s32 defaultValue = -1);
f32 toF32(std::string_view input, f32 defaultValue = -1.0f);
f64 toF64(std::string_view input, f64 defaultValue = -1.0);

std::string toText(bytearray rawBuffer);

///
/// \brief Slice a string into a pair via the first occurence of `delimiter`
///
std::pair<std::string, std::string> bisect(std::string_view input, char delimiter);
///
/// \brief Remove all occurrences of `toRemove` from `outInput`
///
void removeChars(std::string& outInput, std::initializer_list<char> toRemove);
///
/// \brief Remove leading and trailing characters
///
void trim(std::string& outInput, std::initializer_list<char> toRemove);
///
/// \brief Remove all tabs and spaces
///
void removeWhitespace(std::string& outInput);
///
/// \brief Tokenise a string via `delimiter`, skipping over any delimiters within `escape` characters
///
std::vector<std::string> tokenise(std::string_view s, char delimiter, std::initializer_list<std::pair<char, char>> escape);
///
/// \brief Tokenise a string in place via `delimiter`, skipping over any delimiters within `escape` characters
///
std::vector<std::string_view> tokeniseInPlace(char* szOutBuf, char delimiter, std::initializer_list<std::pair<char, char>> escape);

///
/// \brief Substitute an input set of chars with a given replacement
///
void substituteChars(std::string& out_input, std::initializer_list<std::pair<char, char>> replacements);
///
/// \brief Check if `str` is enclosed in a pair of `char`s
/// \returns `true` if `str[idx - 1] = wrapper.first && str[idx + 1] == wrapper.second`
///
bool isCharEnclosedIn(std::string_view str, size_t idx, std::pair<char, char> wrapper);
} // namespace strings
} // namespace utils

template <typename T, T Zero>
TSemaphore<T, Zero>::Handle::Handle(T& counter, std::condition_variable& condVar, std::mutex& mutex)
	: m_pMutex(&mutex), m_pCV(&condVar), m_pCounter(&counter)
{
	std::scoped_lock lock(*m_pMutex);
	++*m_pCounter;
}

template <typename T, T Zero>
TSemaphore<T, Zero>::Handle::~Handle()
{
	ASSERT(m_pCV, "condition variable is null!");
	std::scoped_lock lock(*m_pMutex);
	--*m_pCounter;
	m_pCV->notify_one();
}

template <typename T, T Zero>
typename TSemaphore<T, Zero>::Handle TSemaphore<T, Zero>::handle() const
{
	return Handle(m_counter, m_cv, m_mutex);
}

template <typename T, T Zero>
bool TSemaphore<T, Zero>::isIdle() const
{
	std::scoped_lock<std::mutex> lock(m_mutex);
	return m_counter == Zero;
}

template <typename T, T Zero>
void TSemaphore<T, Zero>::waitIdle()
{
	std::unique_lock lock(m_mutex);
	m_cv.wait(lock, [this]() { return m_counter == 0; });
	return;
}
} // namespace le
