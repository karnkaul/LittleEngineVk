#pragma once
#include <core/not_null.hpp>
#include <core/utils/expect.hpp>
#include <dumb_tasks/scheduler.hpp>
#include <engine/systems/system.hpp>

namespace le {
class Systems {
  public:
	using ID = std::size_t;

	struct Group {
		std::string name;
		std::vector<not_null<System*>> systems;
	};

	bool attach(not_null<System*> system, std::optional<ID> group = std::nullopt);
	ID addGroup(std::vector<not_null<System*>> systems = {}, std::string name = {});
	Group* findGroup(ID id) noexcept;
	Group const* findGroup(ID id) const noexcept;
	bool containsGroup(ID id) const noexcept { return findGroup(id) != nullptr; }
	void removeGroup(ID id);

	std::size_t size() const noexcept;
	bool empty() const noexcept;
	void clear() noexcept { m_entries.clear(); }

	void tick(dens::registry const& registry, Time_s dt);

  private:
	struct Entry {
		Group group;
		ID id;
	};

	Entry& addEntry(Group&& group);

	std::vector<Entry> m_entries;
	dts::scheduler m_scheduler;
	ID m_nextID{};
};
} // namespace le
