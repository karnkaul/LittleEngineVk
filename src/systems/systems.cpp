#include <core/utils/expect.hpp>
#include <engine/systems/systems.hpp>
#include <map>

namespace le {
void Systems::tick(dens::registry const& registry, Time_s dt) {
	for (auto const& [group, _] : m_entries) {
		for (auto const& system : group.systems) { system->update(m_scheduler, registry, dt); }
	}
}

bool Systems::attach(not_null<System*> system, std::optional<ID> group) {
	if (m_entries.empty()) { addEntry({.name = "(default)", .systems = {}}); }
	if (auto g = group ? findGroup(*group) : &m_entries.back().group) {
		g->systems.push_back(system);
		return true;
	}
	return false;
}

Systems::ID Systems::addGroup(std::vector<not_null<System*>> systems, std::string name) {
	return addEntry({.name = std::move(name), .systems = std::move(systems)}).id;
}

std::size_t Systems::size() const noexcept {
	std::size_t ret{};
	for (auto const& [group, _] : m_entries) { ret += group.systems.size(); }
	return ret;
}

bool Systems::empty() const noexcept {
	return std::all_of(m_entries.begin(), m_entries.end(), [](Entry const& e) { return e.group.systems.empty(); });
}

Systems::Group* Systems::findGroup(ID id) noexcept {
	auto it = std::find_if(m_entries.begin(), m_entries.end(), [id](Entry const& e) { return e.id == id; });
	return it != m_entries.end() ? &it->group : nullptr;
}

Systems::Group const* Systems::findGroup(ID id) const noexcept { return const_cast<Systems&>(*this).findGroup(id); }

void Systems::removeGroup(ID id) {
	std::erase_if(m_entries, [id](Entry const& e) { return e.id == id; });
}

Systems::Entry& Systems::addEntry(Group&& group) {
	m_entries.push_back({.group = std::move(group), .id = m_nextID++});
	return m_entries.back();
}
} // namespace le
