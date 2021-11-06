#include <dumb_ecf/registry2.hpp>

namespace decf {
registry2::registry2() noexcept : m_id(++s_next_id) {}

registry2::record& registry2::get_or_make(entity e) {
	auto it = m_records.find(e);
	if (it == m_records.end()) {
		auto [i, _] = m_records.emplace(e, record{});
		it = i;
	}
	return it->second;
}

void registry2::migrate(record& out_record, detail::archetype& out_arch) {
	send_to_back(out_record);
	auto popped = out_record.arch->migrate_back(out_arch);
	auto& record = m_records[popped];
	assert(record.arch == out_record.arch);
	record.arch = &out_arch;
	// record.index = out_arch.size(); must be done by caller
}

void registry2::send_to_back(record& r) {
	if (!r.arch->is_last(r.index)) {
		// swap with last
		entity displaced = r.arch->swap_back(r.index);
		// reindex displaced
		assert(m_records[displaced].arch == r.arch);
		m_records[displaced].index = r.index;
	}
}
} // namespace decf
