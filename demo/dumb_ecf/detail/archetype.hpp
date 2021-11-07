#pragma once
#include <tuple>
#include <dumb_ecf/detail/tarray.hpp>
#include <dumb_ecf/entity.hpp>

namespace decf {
template <typename... Types>
using slice_t = std::tuple<Types&...>;

namespace detail {
class archetype {
  public:
	struct id_t {
		std::vector<sign_t> types;
		sign_t combined{};

		static id_t make(std::span<sign_t const> signs) {
			id_t ret;
			ret.types = {signs.begin(), signs.end()};
			ret.combined = sign_t::combine(signs);
			return ret;
		}

		id_t make(sign_t remove) const {
			auto ret = types;
			for (auto it = ret.begin(); it != ret.end(); ++it) {
				if (*it == remove) {
					ret.erase(it);
					return make(ret);
				}
			}
			return *this;
		}

		bool operator==(id_t const& rhs) const noexcept { return combined == rhs.combined; }

		struct hasher {
			std::size_t operator()(id_t const& id) const noexcept { return id.combined; }
		};
	};

	static archetype make(tarray_factory const& factory, std::span<sign_t const> signs) {
		archetype ret;
		ret.m_id = id_t::make(signs);
		ret.m_arrays.reserve(signs.size());
		for (auto const sign : signs) { ret.m_arrays.push_back(factory.make_tarray(sign)); }
		return ret;
	}

	id_t const& id() const noexcept { return m_id; }
	std::size_t size() const noexcept { return m_arrays.empty() ? 0 : m_arrays[0]->size(); }
	bool empty() const noexcept { return size() == 0; }

	tarray_base* find_base(sign_t sign) const noexcept {
		for (auto const& r : m_arrays) {
			if (r->match(sign)) { return r.get(); }
		}
		return {};
	}

	bool has_any(std::span<sign_t const> signs) const noexcept {
		for (auto const sign : signs) {
			if (find_base(sign)) { return true; }
		}
		return false;
	}

	template <typename T>
	tarray<T>* find() const noexcept {
		if (auto ret = find_base(sign_t::make<T>())) { return static_cast<tarray<T>*>(ret); }
		return {};
	}

	template <typename T>
	tarray<T>& get() const {
		auto found = find<T>();
		assert(found);
		return *found;
	}

	template <typename... Types>
	slice_t<Types...> at(std::size_t index) const {
		return std::tie(get<Types>().m_storage.at(index)...);
	}

	template <typename... Types>
	slice_t<Types...> push_array(Types&&... args) {
		(get<Types>().m_storage.push_back(std::forward<Types>(args)), ...);
		return at<Types...>(size() - 1);
	}

	bool is_last(std::size_t index) const noexcept { return index + 1 == size(); }

	entity swap_back(std::size_t index) {
		assert(index < size() && !is_last(index));
		std::swap(m_entities.at(index), m_entities.back());
		for (auto& array : m_arrays) { array->swap_back(index); }
		return m_entities.at(index);
	}

	entity migrate_back(archetype& target) {
		auto const ret = m_entities.back();
		m_entities.pop_back();
		for (auto& array : m_arrays) { array->pop_push_back(target.find_base(array->sign())); }
		target.m_entities.push_back(ret);
		return ret;
	}

	void pop_back() {
		for (auto& array : m_arrays) { array->pop_back(); }
		m_entities.pop_back();
	}

	// must push exactly one entity before any components
	// precondition: entity count must equal count of first component (or 0 if none)
	void push_back(entity e) {
		[[maybe_unused]] std::size_t const size = m_arrays.empty() ? 0 : m_arrays[0]->size();
		assert(m_entities.size() == size);
		m_entities.push_back(e);
	}

	// must have pushed exactly one entity before any components
	// postcondition: entity count must equal count of modified component
	template <typename T, typename... Args>
	std::vector<T>& emplace_back(Args&&... args) {
		auto& vec = get<T>().m_storage;
		vec.emplace_back(std::forward<Args>(args)...);
		assert(m_entities.size() == vec.size());
		return vec;
	}

	bool contains(entity e) const noexcept {
		for (auto const& entity : m_entities) {
			if (entity == e) { return true; }
		}
		return false;
	}

  private:
	std::vector<std::unique_ptr<tarray_base>> m_arrays;
	std::vector<entity> m_entities; // must be index-locked to m_arrays[0]
	id_t m_id;
};

class archetype_map {
  public:
	using id_t = archetype::id_t;

	template <typename... Types>
	void register_types() {
		(m_factory.register_type<Types>(), ...);
	}

	bool registered(sign_t sign) const noexcept { return m_factory.registered(sign); }

	archetype& get_or_make(std::span<sign_t const> signs) {
		auto const id = id_t::make(signs);
		auto& ret = m_map[id];
		if (ret.id() == id_t{}) { ret = archetype::make(m_factory, signs); }
		return ret;
	}

	template <typename T>
	archetype& copy_append(archetype const& rhs) {
		auto signs = rhs.id().types;
		signs.push_back(sign_t::make<T>());
		return get_or_make(signs);
	}

	template <typename... Types>
	archetype const* find() const noexcept {
		static auto const combined = sign_t::make<Types...>().combined;
		if (auto it = m_map.find(combined); it != m_map.end()) { return &it->second; }
		return {};
	}

	using storage_map = std::unordered_map<id_t, archetype, id_t::hasher>;
	storage_map m_map;
	tarray_factory m_factory;
};
} // namespace detail
} // namespace decf
