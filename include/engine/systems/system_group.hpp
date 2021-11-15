#pragma once
#include <core/utils/expect.hpp>
#include <engine/systems/system.hpp>
#include <concepts>
#include <unordered_map>

namespace le {
template <typename T>
concept System_t = std::is_base_of_v<System, T>;

class SystemGroup : public System {
  public:
	using Sign = dens::detail::sign_t;
	using Order = System::Order;

	template <System_t S, typename... Args>
	S& attach(Order order = 0, Args&&... args);

	template <System_t S>
	S* find() const noexcept;

	template <System_t S>
	bool attached() const noexcept;

	template <System_t S>
	void detach();

	template <System_t S>
	bool reorder(Order order);

	void clear() noexcept { m_entries.clear(); }
	std::size_t size() const noexcept { return m_entries.size(); }
	bool empty() const noexcept { return m_entries.empty(); }

  private:
	void tick(dens::registry const& registry, Time_s dt) override;

	struct Entry {
		std::unique_ptr<System> system;
		Order order{};
	};

	std::unordered_map<Sign, Entry, Sign::hasher> m_entries;
};

// impl

template <System_t S, typename... Args>
S& SystemGroup::attach(Order order, Args&&... args) {
	EXPECT(!attached<S>());
	auto s = std::make_unique<S>(std::forward<Args>(args)...);
	auto& ret = *s;
	m_entries.insert_or_assign(Sign::make<S>(), Entry{.system = std::move(s), .order = order});
	return ret;
}

template <System_t S>
S* SystemGroup::find() const noexcept {
	if (auto it = m_entries.find(Sign::make<S>()); it != m_entries.end()) { return static_cast<S*>(it->second.system.get()); }
	return {};
}

template <System_t S>
bool SystemGroup::attached() const noexcept {
	return find<S>() != nullptr;
}

template <System_t S>
void SystemGroup::detach() {
	m_entries.erase(Sign::make<S>());
}

template <System_t S>
bool SystemGroup::reorder(Order order) {
	if (auto s = find<S>()) {
		s->order = order;
		return true;
	}
	return false;
}
} // namespace le
