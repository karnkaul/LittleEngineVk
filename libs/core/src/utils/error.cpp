#include <core/log.hpp>
#include <core/os.hpp>
#include <core/utils/error.hpp>

namespace le {
void utils::OnError::unsetActive(bool force) noexcept {
	if (force || isActive()) { s_active = {}; }
}

void utils::OnError::dispatch(std::string_view message, SrcInfo const& source) {
	if (s_active) { (*s_active)(message, source); }
}

utils::OnError::Scoped::Scoped(std::string_view msg, SrcInfo const& src) : m_msg(msg), m_src(src) {
	std::string_view const pre = msg.empty() ? "Error " : "Error: ";
	src.logMsg(pre.data(), msg.data(), dl::level::error);
}

utils::OnError::Scoped::~Scoped() { dispatch(m_msg, m_src); }
} // namespace le
