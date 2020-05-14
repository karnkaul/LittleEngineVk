#pragma once
#include <future>
#include <string>
#include <vector>
#include <typeinfo>
#include <type_traits>
#include "core/std_types.hpp"

namespace le
{
enum class FutureState : u8
{
	eInvalid,
	eDeferred,
	eReady,
	eTimeout,
	eCOUNT_
};

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
		auto const status = future.wait_for(std::chrono::milliseconds(0));
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

std::pair<f32, std::string_view> friendlySize(u64 byteCount);

std::string demangle(std::string_view name);

template <typename T>
std::string tName(T const& t)
{
	return demangle(typeid(t).name());
}

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

// Slices a string into a pair via the first occurence of a delimiter
std::pair<std::string, std::string> bisect(std::string_view input, char delimiter);
// Removes all occurrences of toRemove from outInput
void removeChars(std::string& outInput, std::initializer_list<char> toRemove);
// Removes leading and trailing characters
void trim(std::string& outInput, std::initializer_list<char> toRemove);
// Removes all tabs and spaces
void removeWhitespace(std::string& outInput);
// Tokenises a string via a delimiter, skipping over any delimiters within escape characters
std::vector<std::string> tokenise(std::string_view s, char delimiter, std::initializer_list<std::pair<char, char>> escape);
std::vector<std::string_view> tokeniseInPlace(char* szOutBuf, char delimiter, std::initializer_list<std::pair<char, char>> escape);

// Substitutes an input set of chars with a given replacement
void substituteChars(std::string& outInput, std::initializer_list<std::pair<char, char>> replacements);
// Returns true if str[idx - 1] = wrapper.first && str[idx + 1] == wrapper.second
bool isCharEnclosedIn(std::string_view str, size_t idx, std::pair<char, char> wrapper);
} // namespace strings
} // namespace utils
} // namespace le
