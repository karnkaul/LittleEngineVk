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
class OnError {
  public:
	OnError() = default;
	OnError(OnError&&) = default;
	OnError(OnError const&) = default;
	OnError& operator=(OnError&&) = default;
	OnError& operator=(OnError const&) = default;
	virtual ~OnError() noexcept { unsetActive(); }

	///
	/// \brief Customization point
	///
	virtual void operator()(std::string_view message, SrcInfo const& source) = 0;

	///
	/// \brief Obtain active error handler
	///
	static OnError const* activeHandler() noexcept { return s_active; }
	///
	/// \brief Dispatch to active error handler, if set
	///
	static void dispatch(std::string_view message, SrcInfo const& source);

	///
	/// \brief Set this instance as active error handler
	///
	void setActive() noexcept { s_active = this; }
	///
	/// \brief Unset active handler if this instance (or if force)
	///
	void unsetActive(bool force = false) noexcept;
	///
	/// \brief Check if active handler is this instance
	///
	bool isActive() const noexcept { return activeHandler() == this; }

  private:
	inline static OnError* s_active{};
};

///
/// \brief Log msg as an error, break if debugging, and throw Error
///
void error(std::string msg, char const* fl = __builtin_FILE(), char const* fn = __builtin_FUNCTION(), int ln = __builtin_LINE()) noexcept(false);
} // namespace le::utils
