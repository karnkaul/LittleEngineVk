#pragma once
#include <future>
#include <string>
#include <vector>
#include <typeinfo>
#include <type_traits>
#include "std_types.hpp"

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
/// \brief View-only class for arrays / STL containers / `std::initializer_list`s
///
template <typename T>
struct ArrayView
{
	using value_type = T;
	using const_iterator = T const*;

	T const* pData;
	size_t extent;

	constexpr ArrayView() noexcept : pData(nullptr), extent(0) {}
	constexpr ArrayView(T const* pData, size_t extent) noexcept : pData(pData), extent(extent) {}

	template <typename Container>
	constexpr ArrayView(Container const& container) noexcept
		: pData(container.empty() ? nullptr : &container.front()), extent(container.size())
	{
	}

	constexpr ArrayView(std::initializer_list<T const> initList) noexcept
		: pData(initList.size() == 0 ? nullptr : &(*initList.begin())), extent(initList.size())
	{
	}

	ArrayView<T>(ArrayView<T>&&) noexcept = default;
	ArrayView<T>& operator=(ArrayView<T>&&) noexcept = default;
	ArrayView<T>(ArrayView<T> const&) noexcept = default;
	ArrayView<T>& operator=(ArrayView<T> const&) noexcept = default;

	const_iterator begin() const
	{
		return pData;
	}

	const_iterator end() const
	{
		return pData + extent;
	}
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
} // namespace le
