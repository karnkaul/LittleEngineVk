#pragma once
#include <array>
#include <cstring>
#include <deque>
#include <future>
#include <initializer_list>
#include <mutex>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>
#include <core/ensure.hpp>
#include <core/ref.hpp>
#include <core/std_types.hpp>
#include <kt/async_queue/lockable.hpp>
#include <kt/fixed_vector/fixed_vector.hpp>

namespace le {
enum class FutureState : s8 { eInvalid, eDeferred, eReady, eTimeout, eCOUNT_ };

namespace utils {
///
/// \brief Wrapper for kt::lockable
///
template <bool UseMutex, typename M = std::mutex>
struct Lockable final {
	using type = M;
	static constexpr bool hasMutex = UseMutex;

	mutable kt::lockable<M> mutex;

	template <template <typename...> typename L = std::scoped_lock, typename... Args>
	decltype(mutex.template lock<L, Args...>()) lock() const {
		return mutex.template lock<L, Args...>();
	}
};

///
/// \brief Specialisation for Dummy lock
///
template <typename M>
struct Lockable<false, M> {
	using type = void;
	static constexpr bool hasMutex = false;

	struct Dummy final {
		///
		/// \brief Custom destructor to suppress unused variable warnings
		///
		~Dummy() {
		}
	};

	template <template <typename...> typename = std::scoped_lock, typename...>
	Dummy lock() const {
		return {};
	}
};

///
/// \brief std::future wrapper
///
template <typename T>
struct Future {
	mutable std::future<T> future;

	FutureState state() const;
	bool busy() const;
	bool ready(bool bAllowInvalid) const;
	void wait() const;
};

template <typename T>
FutureState futureState(std::future<T> const& future) noexcept;

template <typename T>
bool ready(std::future<T> const& future) noexcept;

///
/// \brief Convert `byteCount` bytes into human-friendly format
/// \returns pair of size in `f32` and the corresponding unit
///
std::pair<f32, std::string_view> friendlySize(u64 byteCount) noexcept;

///
/// \brief Demangle a compiler symbol name
///
std::string demangle(std::string_view name, bool bMinimal);

///
/// \brief Obtain demangled type name of an object or a type
///
template <typename T, bool Minimal = true>
std::string tName(T const* pT = nullptr) {
	if constexpr (Minimal) {
		return demangle(pT ? typeid(*pT).name() : typeid(T).name(), true);
	} else {
		return demangle(pT ? typeid(*pT).name() : typeid(T).name(), false);
	}
}

///
/// \brief Remove namespace prefixes from a type string
///
void removeNamesapces(std::string& out_name);

namespace strings {
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
/// \brief Tokenise a string in place via `delimiter`
///
template <std::size_t N>
kt::fixed_vector<std::string_view, N> tokenise(std::string_view text, char delim);

///
/// \brief Substitute an input set of chars with a given replacement
///
void substituteChars(std::string& out_input, std::initializer_list<std::pair<char, char>> replacements);
///
/// \brief Check if `str` is enclosed in a pair of `char`s
/// \returns `true` if `str[idx - 1] = wrapper.first && str[idx + 1] == wrapper.second`
///
bool isCharEnclosedIn(std::string_view str, std::size_t idx, std::pair<char, char> wrapper);
} // namespace strings

template <typename T>
FutureState Future<T>::state() const {
	return utils::futureState(future);
}

template <typename T>
bool Future<T>::busy() const {
	return state() == FutureState::eDeferred;
}

template <typename T>
bool Future<T>::ready(bool bAllowInvalid) const {
	if (future.valid()) {
		return state() == FutureState::eReady;
	} else {
		return bAllowInvalid;
	}
}

template <typename T>
void Future<T>::wait() const {
	if (future.valid()) {
		future.get();
	}
}
} // namespace utils

template <typename T>
FutureState utils::futureState(std::future<T> const& future) noexcept {
	if (future.valid()) {
		using namespace std::chrono_literals;
		auto const status = future.wait_for(0ms);
		switch (status) {
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

template <std::size_t N>
kt::fixed_vector<std::string_view, N> utils::strings::tokenise(std::string_view text, char delim) {
	kt::fixed_vector<std::string_view, N> ret;
	while (!text.empty() && ret.size() < ret.capacity()) {
		std::size_t const idx = text.find_first_of(delim);
		if (idx < text.size()) {
			ret.push_back(text.substr(0, idx));
			text = idx + 1 < text.size() ? text.substr(idx + 1) : std::string_view();
		} else {
			ret.push_back(text);
			text = {};
		}
	}
	return ret;
}

template <typename T>
bool utils::ready(std::future<T> const& future) noexcept {
	using namespace std::chrono_literals;
	return future.valid() && future.wait_for(0ms) == std::future_status::ready;
}
} // namespace le

namespace std {
template <typename T>
struct hash<le::Ref<T>> {
	size_t operator()(le::Ref<T> const& lhs) const {
		return std::hash<T const*>()(&lhs.get());
	}
};
} // namespace std
