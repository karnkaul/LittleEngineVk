#pragma once
#include <dens/detail/archetype.hpp>
#include <string>

namespace dens {
template <typename... Types>
struct exclude {
	inline static detail::sign_t const signs[sizeof...(Types)] = {detail::sign_t::make<Types>()...};
};

template <>
struct exclude<> {
	static constexpr std::span<detail::sign_t const> signs = {};
};

class registry {
  public:
	inline static std::string s_name_prefix = "entity_";

	registry() noexcept;
	std::size_t id() const noexcept { return m_id; }

	template <typename... Types>
	entity make_entity(std::string name = {});
	bool contains(entity e) const { return m_records.contains(e); }
	bool destroy(entity e);
	std::string_view name(entity e) const;
	bool rename(entity e, std::string name);
	std::size_t size() const noexcept { return m_records.size(); }
	void clear() noexcept;

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
	std::vector<entity_view<Types...>> view(exclude<Exclude...> = exclude<>{}) const;

  private:
	struct record {
		std::string name;
		detail::archetype* arch{};
		std::size_t index{};
	};

	static std::string make_name(std::size_t id);

	record& get_or_make(entity e);
	template <typename T>
	void emplace_back(record& r, detail::archetype& arch);
	void migrate_to(record& out_record, detail::archetype* out_arch);
	void send_to_back(record& r);
	template <typename T>
	bool do_detach(entity e);
	template <typename... T>
	void append(std::vector<entity_view<T...>>& out, detail::archetype const& arch) const;

	inline static std::size_t s_next_id{};

	detail::archetype_map m_map;
	std::unordered_map<entity, record, entity::hasher> m_records;
	std::size_t m_next_id{};
	std::size_t m_id{};
};

// impl

inline registry::registry() noexcept : m_id(++s_next_id) {}

template <typename... Types>
entity registry::make_entity(std::string name) {
	auto const id = ++m_next_id;
	if (name.empty()) { name = make_name(id); }
	auto [it, _] = m_records.emplace(entity{id, m_id}, record{std::move(name)});
	if constexpr (sizeof...(Types) > 0) {
		detail::sign_t const signs[] = {detail::sign_t::make<Types>()...};
		m_map.register_types<Types...>();
		detail::archetype& arch = m_map.get_or_make(signs);
		arch.push_back(it->first);
		(emplace_back<Types>(it->second, arch), ...);
	}
	return it->first;
}

inline std::string_view registry::name(entity e) const {
	if (auto it = m_records.find(e); it != m_records.end()) { return it->second.name; }
	return {};
}

inline bool registry::destroy(entity e) {
	if (auto it = m_records.find(e); it != m_records.end()) {
		auto& r = it->second;
		if (r.arch) { migrate_to(r, nullptr); }
		m_records.erase(it);
		return true;
	}
	return false;
}

inline bool registry::rename(entity e, std::string name) {
	if (auto it = m_records.find(e); it != m_records.end()) {
		it->second.name = std::move(name);
		return true;
	}
	return false;
}

inline void registry::clear() noexcept {
	m_map.m_map.clear();
	m_records.clear();
}

template <typename T, typename... Args>
T& registry::attach(entity e, Args&&... args) {
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
		migrate_to(rec, &m_map.copy_append<T>(*rec.arch));
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
bool registry::attached(entity e) const {
	if (auto it = m_records.find(e); it != m_records.end() && it->second.arch) { return it->second.arch->find<T>(); }
	return false;
}

template <typename T>
T* registry::find(entity e) const {
	if (auto it = m_records.find(e); it != m_records.end() && it->second.arch) {
		record const& r = it->second;
		if (auto const& array = r.arch->find<T>()) { return &array->m_storage.at(r.index); }
	}
	return {};
}

template <typename T>
T& registry::get(entity e) const {
	assert(e.registry_id == m_id);
	auto ret = find<T>(e);
	assert(ret);
	return *ret;
}

template <typename... Types, typename... Exclude>
std::vector<entity_view<Types...>> registry::view(exclude<Exclude...>) const {
	std::vector<entity_view<Types...>> ret;
	for (auto const& [_, arch] : m_map.m_map) {
		if ((arch.find<Types>() && ...) && !arch.has_any(exclude<Exclude...>::signs)) { append(ret, arch); }
	}
	return ret;
}

inline registry::record& registry::get_or_make(entity e) {
	auto it = m_records.find(e);
	if (it == m_records.end()) {
		auto [i, _] = m_records.emplace(e, record{});
		it = i;
	}
	return it->second;
}

template <typename T>
void registry::emplace_back(record& r, detail::archetype& arch) {
	r.arch = &arch;
	auto& vec = r.arch->get<T>().m_storage;
	r.index = vec.size();
	vec.emplace_back();
}

inline void registry::migrate_to(record& out_record, detail::archetype* out_arch) {
	send_to_back(out_record);
	[[maybe_unused]] auto popped = out_record.arch->migrate_back(out_arch);
	assert(&m_records[popped] == &out_record);
	out_record.arch = out_arch;
	// record.index = out_arch.size(); must be done by caller
}

inline void registry::send_to_back(record& r) {
	if (!r.arch->is_last(r.index)) {
		// swap with last
		entity displaced = r.arch->swap_back(r.index);
		// reindex displaced
		assert(m_records[displaced].arch == r.arch);
		m_records[displaced].index = r.index;
	}
}

template <typename T>
bool registry::do_detach(entity e) {
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
		[[maybe_unused]] entity migrated = rec.arch->migrate_back(&target);
		assert(&m_records[migrated] == &rec);
		rec.arch = &target;
		assert(!target.empty());
		rec.index = target.size() - 1;
	}
	return true;
}

template <typename... T>
void registry::append(std::vector<entity_view<T...>>& out, detail::archetype const& arch) const {
	std::size_t const size = arch.size();
	out.reserve(out.size() + size);
	for (std::size_t i = 0; i < size; ++i) { out.push_back(arch.at<T...>(i)); }
}

inline std::string registry::make_name(std::size_t id) {
	std::string ret = s_name_prefix;
	ret += std::to_string(id);
	return ret;
}
} // namespace dens
