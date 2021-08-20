#pragma once
#include <stdexcept>
#include <string>

namespace le::utils {
///
/// \brief Source information
///
struct SrcInfo {
	std::string_view function = "(unknown)";
	std::string_view file = "(unknown)";
	int line = 0;
};

///
/// \brief Error exception type
///
struct Error : std::runtime_error {
	Error(std::string_view msg) : std::runtime_error(msg.data()) {}
};

///
/// \brief Abstract interface for customization point
///
struct OnError {
	virtual void operator()(std::string_view message, SrcInfo const& source) const = 0;
};

///
/// \brief Customization point (invoked before Error is thrown)
///
inline OnError const* g_onError = {};

///
/// \brief Log msg as an error, break if debugging, and throw Error
///
void error(std::string msg, char const* fl = __builtin_FILE(), char const* fn = __builtin_FUNCTION(), int ln = __builtin_LINE()) noexcept(false);
} // namespace le::utils
