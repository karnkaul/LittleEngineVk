#pragma once
#include <core/utils/debug.hpp>
#include <core/utils/src_info.hpp>
#include <stdexcept>
#include <string>

#if defined(DISPATCH_ERROR)
#undef DISPATCH_ERROR
#endif
#define DISPATCH_ERROR(msg)                                                                                                                                    \
	do {                                                                                                                                                       \
		::le::utils::OnError::Scoped s(msg, {__func__, __FILE__, __LINE__});                                                                                   \
		DEBUG_TRAP();                                                                                                                                          \
	} while (false)

#if defined(ENSURE)
#undef ENSURE
#endif
#define ENSURE(pred, msg)                                                                                                                                      \
	if (!(pred)) {                                                                                                                                             \
		DISPATCH_ERROR(msg);                                                                                                                                   \
		throw ::le::utils::Error(msg);                                                                                                                         \
	}

namespace le::utils {
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

	class Scoped;

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

class OnError::Scoped {
  public:
	Scoped(std::string_view msg, SrcInfo const& src);
	~Scoped();

  private:
	std::string_view m_msg;
	SrcInfo m_src;
};
} // namespace le::utils
