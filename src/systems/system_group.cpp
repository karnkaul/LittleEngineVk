#include <engine/systems/system_group.hpp>

namespace le {
void SystemGroup::tick(dens::registry const& registry, Time_s dt) {
	if (m_entries.size() < 2) {
		for (auto& [_, entry] : m_entries) { entry.system->update(*m_scheduler, registry, dt); }
		return;
	}
	std::vector<Entry*> sorted;
	sorted.reserve(m_entries.size());
	for (auto& [_, entry] : m_entries) { sorted.push_back(&entry); }
	std::sort(sorted.begin(), sorted.end(), [](Entry const* l, Entry const* r) { return l->order < r->order; });
	for (Entry* entry : sorted) { entry->system->update(*m_scheduler, registry, dt); }
}
} // namespace le
