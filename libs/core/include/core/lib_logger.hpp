#pragma once
#include <core/log.hpp>

namespace le {
struct LibLogger {
	///
	/// \brief The target audience of a log message
	///
	enum class Verbosity { eEndUser, eLibUser, eLibrary };

	static constexpr Verbosity libVerbosity = levk_pre_release ? Verbosity::eLibrary : Verbosity::eEndUser;

	Verbosity minVerbosity = Verbosity::eLibUser;

	template <typename... Args>
	void log(dl::level level, Verbosity verbosity, std::string_view fmt, Args&&... args) {
		if (verbosity <= minVerbosity) {
			::le::detail::logImpl(level, fmt, std::forward<Args>(args)...);
		}
	}

	template <typename... Args>
	void log(dl::level level, int verbosity, std::string_view fmt, Args&&... args) {
		log(level, static_cast<Verbosity>(verbosity), fmt, std::forward<Args>(args)...);
	}
};
} // namespace le
