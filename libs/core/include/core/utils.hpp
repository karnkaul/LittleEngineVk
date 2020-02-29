#pragma once
#include <string>
#include <vector>
#if defined(__GNUG__)
#include <cxxabi.h>
#endif
#include "core/std_types.hpp"

namespace le
{
template <typename Type>
struct Span
{
	Type* pData;
	size_t extent;

	constexpr Span() noexcept : pData(nullptr), extent(0) {}
	constexpr Span(Type* pData, size_t extent) noexcept : pData(pData), extent(extent) {}

	template <typename Container>
	constexpr explicit Span(Container& container) noexcept
		: pData(container.empty() ? nullptr : &container.front()), extent(container.size())
	{
	}
};

namespace utils
{
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
