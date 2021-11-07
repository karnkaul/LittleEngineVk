#pragma once
#include <dumb_ecf/view.hpp>

namespace decf {
class registry2 {
  public:
	registry2() noexcept;
	std::size_t id() const noexcept { return m_id; }

	template <typename... Types>
	entity make_entity();

	bool contains(entity e) const { return m_records.contains(e); }

	template <typename T, typename... Args>
	T& attach(entity e, Args&&... args);

	template <typename... Types>
		requires(sizeof...(Types) > 1)
	void attach(entity e) { (attach<Types>(e), ...); }

	template <typename T>
	bool attached(entity e) const;

	template <typename... Types>
		requires(sizeof...(Types) > 0)
	bool detach(entity e) { return (do_detach<Types>(e) && ...); }

	template <typename T>
	T* find(entity e) const;

	template <typename T>
	T& get(entity e) const;

	template <typename... Types, typename... Exclude>
	std::vector<array_view<Types...>> view(exclude<Exclude...> = exclude<>{}) const;

  private:
	struct record {
		detail::archetype* arch{};
		std::size_t index{};
	};

	record& get_or_make(entity e);
	template <typename T>
	void emplace_back(record& r, detail::archetype& arch);
	void migrate(record& out_record, detail::archetype& out_arch);
	void send_to_back(record& r);
	template <typename T>
	bool do_detach(entity e);

	inline static std::size_t s_next_id{};

	detail::archetype_map m_map;
	std::unordered_map<entity, record, entity::hasher> m_records;
	std::size_t m_next_id{};
	std::size_t m_id{};
};

// impl

inline registry2::registry2() noexcept : m_id(++s_next_id) {}

template <typename... Types>
entity registry2::make_entity() {
	auto [it, _] = m_records.emplace(entity{++m_next_id, m_id}, record{});
	if constexpr (sizeof...(Types) > 0) {
		detail::sign_t const signs[] = {detail::sign_t::make<Types>()...};
		m_map.register_types<Types...>();
		detail::archetype& arch = m_map.get_or_make(signs);
		arch.push_back(it->first);
		(emplace_back<Types>(it->second, arch), ...);
	}
	return it->first;
}

template <typename T, typename... Args>
T& registry2::attach(entity e, Args&&... args) {
	assert(e.registry_id == m_id);
	m_map.register_types<T>();
	record& rec = get_or_make(e);
	if (rec.arch) {
		if (auto array = rec.arch->find<T>()) {
			// component T already exists, move assign and return
			array->m_storage.at(rec.index) = T{std::forward<Args>(args)...};
			return array->m_storage.at(rec.index);
		}
		// migrate record to archetype with existing components + T, to be pushed
		migrate(rec, m_map.copy_append<T>(*rec.arch));
	} else {
		// no existing components, use archetype with only T, to be pushed
		detail::sign_t const signs[] = {detail::sign_t::make<T>()};
		rec.arch = &m_map.get_or_make(signs);
		rec.arch->push_back(e);
	}
	assert(rec.arch);
	// push new T instance
	auto& vec = rec.arch->emplace_back<T>(std::forward<Args>(args)...);
	assert(!vec.empty());
	// update record index
	rec.index = vec.size() - 1;
	assert(vec.size() == rec.arch->size());
	return vec.back();
}

template <typename T>
bool registry2::attached(entity e) const {
	if (auto it = m_records.find(e); it != m_records.end()) { return it->second.arch && it->second.arch->find<T>(); }
}

template <typename T>
T* registry2::find(entity e) const {
	if (auto it = m_records.find(e); it != m_records.end() && it->second.arch) {
		record const& r = it->second;
		if (auto const& array = r.arch->find<T>()) { return &array->m_storage.at(r.index); }
	}
	return {};
}

template <typename T>
T& registry2::get(entity e) const {
	assert(e.registry_id == m_id);
	auto ret = find<T>(e);
	assert(ret);
	return *ret;
}

template <typename... Types, typename... Exclude>
std::vector<array_view<Types...>> registry2::view(exclude<Exclude...>) const {
	std::vector<array_view<Types...>> ret;
	for (auto const& [_, arch] : m_map.m_map) {
		if ((arch.find<Types>() && ...) && !arch.has_any(exclude<Exclude...>::signs)) { ret.push_back(&arch); }
	}
	return ret;
}

inline registry2::record& registry2::get_or_make(entity e) {
	auto it = m_records.find(e);
	if (it == m_records.end()) {
		auto [i, _] = m_records.emplace(e, record{});
		it = i;
	}
	return it->second;
}

template <typename T>
void registry2::emplace_back(record& r, detail::archetype& arch) {
	r.arch = &arch;
	auto& vec = r.arch->get<T>().m_storage;
	r.index = vec.size();
	vec.emplace_back();
}

inline void registry2::migrate(record& out_record, detail::archetype& out_arch) {
	send_to_back(out_record);
	[[maybe_unused]] auto popped = out_record.arch->migrate_back(out_arch);
	assert(&m_records[popped] == &out_record);
	out_record.arch = &out_arch;
	// record.index = out_arch.size(); must be done by caller
}

inline void registry2::send_to_back(record& r) {
	if (!r.arch->is_last(r.index)) {
		// swap with last
		entity displaced = r.arch->swap_back(r.index);
		// reindex displaced
		assert(m_records[displaced].arch == r.arch);
		m_records[displaced].index = r.index;
	}
}
template <typename T>
bool registry2::do_detach(entity e) {
	if (e.registry_id != m_id) { return false; }
	auto it = m_records.find(e);
	if (it == m_records.end() || !it->second.arch) { return false; }
	record& rec = it->second;
	if (!rec.arch->is_last(rec.index)) {
		auto swapped = rec.arch->swap_back(rec.index);
		m_records[swapped].index = rec.index;
	}
	if (rec.arch->id().types.size() == 1) {
		rec.arch->pop_back();
		rec = {};
	} else {
		auto id = rec.arch->id().make(detail::sign_t::make<T>());
		assert(id != rec.arch->id());
		detail::archetype& target = m_map.get_or_make(id.types);
		[[maybe_unused]] entity migrated = rec.arch->migrate_back(target);
		assert(&m_records[migrated] == &rec);
		rec.arch = &target;
		assert(!target.empty());
		rec.index = target.size() - 1;
	}
	return true;
}
} // namespace decf
