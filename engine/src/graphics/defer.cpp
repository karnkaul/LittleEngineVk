#include <spaced/core/logger.hpp>
#include <spaced/graphics/defer.hpp>

namespace spaced::graphics {
namespace {
auto const g_log{logger::Logger{"Defer"}};
}

void DeferQueue::next_frame() {
	auto lock = std::scoped_lock{m_mutex};
	auto count = std::size_t{};
	m_deferred.push_back(std::move(m_current));
	while (m_deferred.size() > buffering_v) {
		count += m_deferred.front().size();
		m_deferred.pop_front();
	}
	if (count > 0) { g_log.debug("{} deferred items destroyed", count); }
}

void DeferQueue::clear() {
	auto lock = std::scoped_lock{m_mutex};
	m_current.clear();
	m_deferred.clear();
}
} // namespace spaced::graphics
